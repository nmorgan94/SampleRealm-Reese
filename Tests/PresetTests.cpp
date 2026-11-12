// PresetManager's own tests are in SampleRealm-Modules. These are this plugin's presets and state
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "A reopened session shows the same preset", "[presets][state]")
{
    // As a session saved with a user preset loaded, since a fresh one shows Init
    auto& presets = processor.getPresetManager();
    auto session = processor.getAPVTS().copyState();
    session.setProperty ("presetName", "Wide", nullptr);
    session.setProperty ("presetCategory", "Bass", nullptr);
    session.setProperty ("presetIsFactory", false, nullptr);
    session.setProperty ("presetModified", false, nullptr);
    presets.restoreState (session);
    REQUIRE (presets.getCurrent().name == "Wide");

    setParameter (processor, Parameters::cutoffId, 900.0f);

    juce::MemoryBlock state;
    processor.getStateInformation (state);

    AudioPluginAudioProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

    const auto current = restored.getPresetManager().getCurrent();
    CHECK (current.name == "Wide");
    CHECK (current.category == "Bass");
    CHECK_FALSE (current.isFactory);
    CHECK (restored.getPresetManager().isModified());
    CHECK_THAT (getParameter (restored, Parameters::cutoffId), WithinAbs (900.0, 1.0e-2));

    // Kept by the preset manager, not left on the parameters
    CHECK_FALSE (restored.getAPVTS().state.hasProperty ("presetName"));
}

TEST_CASE_METHOD (ProcessorFixture, "The plugin's factory presets load every parameter, in range", "[presets]")
{
    auto& presets = processor.getPresetManager();

    // The same order as webui/test/demo/factoryPresets.test.ts
    CHECK (namesOf (presets.getFactoryPresets()) == juce::StringArray { "Classic Reese", "Glide Reese", "Wide Reese",
                                                                         "Deep Rumble", "Night Drive", "Sub Weight",
                                                                         "Grind", "Tearout",
                                                                         "Eighth Wobble", "Slow Sweep", "Triplet Wobble" });

    for (const auto& file : AudioPluginAudioProcessor::createPresetConfig().factoryPresets)
    {
        const auto xml = juce::parseXML (file.toString());
        REQUIRE (xml != nullptr);
        CAPTURE (xml->getStringAttribute ("name"));
        REQUIRE (presets.loadPreset (presets.find (xml->getStringAttribute ("name"), true).value()));

        const auto* values = xml->getChildByName (processor.getAPVTS().state.getType());
        REQUIRE (values != nullptr);
        juce::StringArray ids;

        for (const auto* param : values->getChildIterator())
        {
            const auto value = param->getDoubleAttribute ("value");
            ids.addIfNotAlreadyThere (param->getStringAttribute ("id"));
            CAPTURE (ids.strings.getLast(), value);

            const auto* parameter = processor.getAPVTS().getParameter (ids.strings.getLast());
            REQUIRE (parameter != nullptr);
            CHECK_THAT (parameter->convertFrom0to1 (parameter->getValue()), WithinAbs (value, 1.0e-6) || WithinRel (value, 1.0e-4));
        }

        CHECK (ids.size() == processor.getParameters().size());
    }
}
