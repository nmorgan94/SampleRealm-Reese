#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace
{
    // Stand-ins, so these tests don't follow changes to the plugin's presets
    juce::MemoryBlock factoryPreset (const juce::String& name, const juce::String& category, float cutoff)
    {
        juce::XmlElement preset ("PRESET");
        preset.setAttribute ("name", name);
        preset.setAttribute ("category", category);

        auto* parameters = preset.createNewChildElement ("Parameters");
        auto* param = parameters->createNewChildElement ("PARAM");
        param->setAttribute ("id", Parameters::cutoffId.getParamID());
        param->setAttribute ("value", cutoff);

        const auto text = preset.toString();
        return { text.toRawUTF8(), text.getNumBytesAsUTF8() };
    }

    // Factory presets of the test's own, with user presets in a folder of their own
    struct PresetFixture : ProcessorFixture
    {
        ~PresetFixture()   { userDirectory.deleteRecursively(); }

        juce::File userDirectory = juce::File::createTempFile ("presets");

        PresetManager presets { processor.getAPVTS(), [this]
        {
            auto config = AudioPluginAudioProcessor::createPresetConfig();
            config.userDirectory = userDirectory;
            config.factoryPresets.clear();

            // Out of order, to be sorted
            config.factoryPresets.push_back (factoryPreset ("Dark 2", "Dark", 600.0f));
            config.factoryPresets.push_back (factoryPreset ("Bright 1", "Bright", 8000.0f));
            config.factoryPresets.push_back (factoryPreset ("Dark 10", "Dark", 200.0f));
            config.factoryPresets.push_back (factoryPreset ("Dark 1", "Dark", 300.0f));
            return config;
        }() };

        float cutoff()   { return getParameter (processor, Parameters::cutoffId); }
    };

    juce::StringArray namesOf (const juce::Array<PresetManager::Preset>& list)
    {
        juce::StringArray names;

        for (const auto& preset : list)
            names.add (preset.name);

        return names;
    }

    const juce::StringArray factoryNames { "Bright 1", "Dark 1", "Dark 2", "Dark 10" };
}

//==============================================================================
TEST_CASE_METHOD (PresetFixture, "Factory presets are listed by category, then name", "[presets]")
{
    CHECK (namesOf (presets.getFactoryPresets()) == factoryNames);
    CHECK (namesOf (presets.getPresets()) == factoryNames);

    for (const auto& preset : presets.getFactoryPresets())
    {
        CAPTURE (preset.name);
        CHECK (preset.isFactory);
    }
}

TEST_CASE_METHOD (PresetFixture, "A loaded preset sets its values and shows its name", "[presets]")
{
    CHECK (presets.getCurrent().name == "Init");

    REQUIRE (presets.loadPreset (presets.find ("Dark 1", true).value()));

    CHECK_THAT (cutoff(), WithinAbs (300.0, 1.0e-2));
    CHECK (presets.getCurrent().name == "Dark 1");
    CHECK (presets.getCurrent().category == "Dark");
    CHECK (presets.getCurrent().isFactory);
    CHECK_FALSE (presets.isModified());
}

TEST_CASE_METHOD (PresetFixture, "Moving a parameter marks the preset modified until the next load", "[presets]")
{
    REQUIRE (presets.loadPreset (presets.find ("Bright 1", true).value()));

    setParameter (processor, Parameters::cutoffId, 1000.0f);
    CHECK (presets.isModified());

    REQUIRE (presets.loadPreset (presets.find ("Bright 1", true).value()));
    CHECK_FALSE (presets.isModified());
    CHECK_THAT (cutoff(), WithinAbs (8000.0, 1.0e-2));
}

TEST_CASE_METHOD (PresetFixture, "Changing state outside the parameters marks the preset modified", "[presets]")
{
    REQUIRE (presets.loadPreset (presets.find ("Bright 1", true).value()));

    processor.updateHostDisplay (juce::AudioProcessor::ChangeDetails{}.withProgramChanged (true));
    CHECK_FALSE (presets.isModified());

    processor.updateHostDisplay (juce::AudioProcessor::ChangeDetails{}.withNonParameterStateChanged (true));
    CHECK (presets.isModified());
}

TEST_CASE_METHOD (PresetFixture, "Init restores the defaults", "[presets]")
{
    REQUIRE (presets.loadPreset (presets.find ("Dark 2", true).value()));
    setParameter (processor, Parameters::driveId, 0.9f);

    presets.loadInit();

    CHECK_THAT (cutoff(), WithinAbs (ParameterSpecs::cutoff.defaultValue, 1.0e-2));
    CHECK_THAT (getParameter (processor, Parameters::driveId), WithinAbs (ParameterSpecs::drive.defaultValue, 1.0e-4));
    CHECK (presets.getCurrent().name == "Init");
    CHECK (presets.getCurrent().isFactory);
    CHECK_FALSE (presets.isModified());
}

TEST_CASE_METHOD (PresetFixture, "Next and previous wrap, and start from either end after Init", "[presets]")
{
    REQUIRE (presets.loadNext());
    CHECK (presets.getCurrent().name == factoryNames[0]);

    REQUIRE (presets.loadPrevious());
    CHECK (presets.getCurrent().name == factoryNames[factoryNames.size() - 1]);

    REQUIRE (presets.loadNext());
    CHECK (presets.getCurrent().name == factoryNames[0]);

    presets.loadInit();
    REQUIRE (presets.loadPrevious());
    CHECK (presets.getCurrent().name == factoryNames[factoryNames.size() - 1]);
    CHECK_THAT (cutoff(), WithinAbs (200.0, 1.0e-2));
}

TEST_CASE_METHOD (PresetFixture, "User presets save, rename and delete in their folder", "[presets]")
{
    setParameter (processor, Parameters::cutoffId, 500.0f);

    REQUIRE (presets.saveUserPreset ("Mine", "Dark").wasOk());
    const auto mine = userDirectory.getChildFile ("Mine.srreese");
    CHECK (mine.existsAsFile());
    CHECK (presets.getCurrent().name == "Mine");
    CHECK_FALSE (presets.getCurrent().isFactory);
    CHECK_FALSE (presets.isModified());

    // Listed after the factory presets
    REQUIRE (presets.getPresets().getLast().file == mine);

    presets.loadInit();
    REQUIRE (presets.loadPreset (presets.getPresets().getLast()));
    CHECK_THAT (cutoff(), WithinAbs (500.0, 1.0e-2));
    CHECK (presets.getCurrent().name == "Mine");

    REQUIRE (presets.renameUserPreset (presets.getPresets().getLast(), "Yours").wasOk());
    const auto yours = userDirectory.getChildFile ("Yours.srreese");
    CHECK_FALSE (mine.exists());
    CHECK (yours.existsAsFile());
    CHECK (presets.getCurrent().name == "Yours");
    CHECK (namesOf (presets.getUserPresets()) == juce::StringArray { "Yours" });

    REQUIRE (presets.deleteUserPreset (presets.getPresets().getLast()).wasOk());
    CHECK_FALSE (yours.exists());
    CHECK (presets.getUserPresets().isEmpty());
    CHECK (presets.isModified());
}

TEST_CASE_METHOD (PresetFixture, "User presets can't take a factory name", "[presets]")
{
    CHECK (presets.saveUserPreset ("dark 1", {}).failed());
    CHECK (presets.saveUserPreset ("   ", {}).failed());
    CHECK_FALSE (userDirectory.getChildFile ("dark 1.srreese").exists());

    REQUIRE (presets.saveUserPreset ("Mine", {}).wasOk());
    CHECK (presets.renameUserPreset (presets.getUserPresets().getFirst(), "Dark 2").failed());
    CHECK (userDirectory.getChildFile ("Mine.srreese").existsAsFile());
}

TEST_CASE_METHOD (PresetFixture, "Next and previous skip a user preset deleted outside the plugin", "[presets]")
{
    REQUIRE (presets.saveUserPreset ("A", {}).wasOk());
    REQUIRE (presets.saveUserPreset ("B", {}).wasOk());
    REQUIRE (presets.loadPreset (presets.find ("A", false).value()));

    REQUIRE (userDirectory.getChildFile ("B.srreese").deleteFile());

    REQUIRE (presets.loadNext());
    CHECK (presets.getCurrent().name == factoryNames[0]);
}

TEST_CASE_METHOD (PresetFixture, "Next and previous step past a preset that won't load", "[presets]")
{
    REQUIRE (presets.saveUserPreset ("A", {}).wasOk());
    REQUIRE (userDirectory.getChildFile ("Broken.srreese").replaceWithText ("not a preset"));

    REQUIRE (presets.loadNext());
    CHECK (presets.getCurrent().name == factoryNames[0]);

    REQUIRE (presets.loadPrevious());
    CHECK (presets.getCurrent().name == "A");
}

TEST_CASE_METHOD (PresetFixture, "Rescanning moves the change counter only when the folder changed", "[presets]")
{
    presets.refreshUserPresets();
    const auto before = presets.getChangeCounter();

    presets.refreshUserPresets();
    CHECK (presets.getChangeCounter() == before);

    REQUIRE (userDirectory.createDirectory().wasOk());
    REQUIRE (userDirectory.getChildFile ("Mine.srreese").replaceWithText ("<PRESET/>"));
    presets.refreshUserPresets();
    CHECK (presets.getChangeCounter() != before);
}

TEST_CASE_METHOD (PresetFixture, "Presets are found by name and kind", "[presets]")
{
    REQUIRE (presets.saveUserPreset ("Mine", {}).wasOk());

    CHECK (presets.find ("Dark 2", true).has_value());
    CHECK (presets.find ("Mine", false)->file == userDirectory.getChildFile ("Mine.srreese"));
    CHECK_FALSE (presets.find ("Dark 2", false).has_value());
    CHECK_FALSE (presets.find ("Mine", true).has_value());
}

TEST_CASE_METHOD (PresetFixture, "State from before presets existed shows Init, modified", "[presets][state]")
{
    REQUIRE (presets.loadPreset (presets.find ("Dark 2", true).value()));

    juce::ValueTree state (processor.getAPVTS().state.getType());
    state.appendChild ({ "PARAM", { { "id", Parameters::cutoffId.getParamID() }, { "value", 4000.0f } } }, nullptr);
    presets.restoreState (state);

    CHECK_THAT (cutoff(), WithinAbs (4000.0, 1.0e-2));
    CHECK (presets.getCurrent().name == "Init");
    CHECK (presets.isModified());
}

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

    // The same order as webui/test/demo/presetState.test.ts
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
