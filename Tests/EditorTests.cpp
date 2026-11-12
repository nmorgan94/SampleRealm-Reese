#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"
#include <EditorFixtures.h>
#include <samplerealm/BuildCommit.h>

using namespace TestHelpers;
using EditorFixtures::descendantsOf;
using Editor = AudioPluginAudioProcessorEditor;

static LabeledSlider& findKnob (Editor& editor, const ParameterSpecs::Spec& spec)
{
    auto* knob = dynamic_cast<LabeledSlider*> (editor.findChildWithID (spec.id));
    REQUIRE (knob != nullptr);
    return *knob;
}

static juce::ToggleButton& findSwitch (Editor& editor, const juce::String& text)
{
    juce::ToggleButton* found = nullptr;

    for (auto* button : descendantsOf<juce::ToggleButton> (editor))
        if (button->getButtonText() == text)
            found = button;

    REQUIRE (found != nullptr);
    return *found;
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "About shows the version and build type, the commit and the UI", "[editor]")
{
    const auto lines = juce::StringArray::fromLines (About::text (processor));

    REQUIRE (lines.size() == 5);
    CHECK (lines[0] == "Version " JucePlugin_VersionString);
   #if JUCE_DEBUG
    CHECK (lines[1] == "Debug build");
   #else
    CHECK (lines[1] == "Release build");
   #endif
    CHECK (lines[2] == juce::String ("Commit ") + buildCommit);
    CHECK (lines[3] == "Undefined");
    CHECK (lines[4] == "Native UI");
}

TEST_CASE_METHOD (ProcessorFixture, "The editor's text is Orbitron", "[editor][font]")
{
    Editor editor (processor);
    auto& lookAndFeel = editor.getLookAndFeel();

    juce::Label regular, bold;
    bold.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    juce::TextButton button;

    CHECK (lookAndFeel.getLabelFont (regular).getTypefacePtr()->getName() == "Orbitron");
    CHECK (lookAndFeel.getLabelFont (bold).getTypefacePtr()->getName() == "Orbitron");
    CHECK (lookAndFeel.getTextButtonFont (button, 24).getTypefacePtr()->getName() == "Orbitron");
    CHECK (lookAndFeel.getPopupMenuFont().getTypefacePtr()->getName() == "Orbitron");

    const auto headers = descendantsOf<HeaderBar> (editor);
    REQUIRE (headers.size() == 1);
    CHECK (headers[0]->titleFont().getTypefacePtr()->getName() == "Orbitron");
}

TEST_CASE_METHOD (ProcessorFixture, "The header's title fits its width and leaves the preset name room", "[editor]")
{
    Editor editor (processor);
    const auto headers = descendantsOf<HeaderBar> (editor);
    REQUIRE (headers.size() == 1);

    const auto titleWidth = headers[0]->titleTextWidth();
    CAPTURE (titleWidth);
    CHECK (titleWidth <= static_cast<float> (Editor::headerLayout.titleWidth));
    CHECK (titleWidth > static_cast<float> (Editor::headerLayout.titleWidth - 8));

    juce::TextButton* name = nullptr;

    for (auto* button : descendantsOf<juce::TextButton> (editor))
        if (button->getButtonText() == "Init")
            name = button;

    REQUIRE (name != nullptr);
    CAPTURE (name->getWidth());
    CHECK (name->getWidth() >= 100);
}

TEST_CASE_METHOD (ProcessorFixture, "Knobs show wave names and upper-case sync rates, as the web editor's dragText", "[editor]")
{
    Editor editor (processor);

    auto& wave = findKnob (editor, ParameterSpecs::oscAWave).getSlider();
    CHECK (wave.getTextFromValue (0.0) == "SQUARE");
    CHECK (wave.getTextFromValue (2.0) == "TRI");

    CHECK (findKnob (editor, ParameterSpecs::lfoSyncRate).getSlider().getTextFromValue (12.0) == "1 BAR");
    CHECK (findKnob (editor, ParameterSpecs::cutoff).getSlider().getTextFromValue (440.0) == "440.00");
}

TEST_CASE_METHOD (ProcessorFixture, "Confirming a knob's name without typing leaves its value, sync rates included", "[editor]")
{
    Editor editor (processor);

    for (const auto* spec : { &ParameterSpecs::lfoSyncRate, &ParameterSpecs::oscAWave, &ParameterSpecs::cutoff })
    {
        auto& knob = findKnob (editor, *spec);
        auto& slider = knob.getSlider();
        const auto labels = descendantsOf<juce::Label> (knob);
        REQUIRE (labels.size() == 1);

        for (const auto value : { slider.getMinimum(), slider.getMaximum() - 2.0, slider.getMaximum() })
        {
            slider.setValue (value, juce::sendNotificationSync);
            const auto before = slider.getValue();
            CAPTURE (spec->id, before, slider.getTextFromValue (before));

            // As the typing box opens and is confirmed
            labels[0]->setText (slider.getTextFromValue (before), juce::sendNotificationSync);
            CHECK_THAT (slider.getValue(), Catch::Matchers::WithinAbs (before, 1.0e-6));
        }
    }
}

TEST_CASE_METHOD (ProcessorFixture, "A choice knob takes its names typed in any case", "[editor]")
{
    Editor editor (processor);

    const auto type = [&editor] (const ParameterSpecs::Spec& spec, const juce::String& text)
    {
        auto& knob = findKnob (editor, spec);
        descendantsOf<juce::Label> (knob).front()->setText (text, juce::sendNotificationSync);
        return knob.getSlider().getValue();
    };

    CHECK_THAT (type (ParameterSpecs::oscAWave, "tri"), Catch::Matchers::WithinAbs (2.0, 1.0e-6));
    CHECK_THAT (type (ParameterSpecs::oscBWave, " Square "), Catch::Matchers::WithinAbs (0.0, 1.0e-6));
    CHECK_THAT (type (ParameterSpecs::lfoSyncRate, "1 bar"), Catch::Matchers::WithinAbs (12.0, 1.0e-6));
}

TEST_CASE_METHOD (ProcessorFixture, "The LFO knob and the filter button follow their switches when the host sets them", "[editor]")
{
    Editor editor (processor);
    auto& rate = findKnob (editor, ParameterSpecs::lfoRate);
    auto& syncRate = findKnob (editor, ParameterSpecs::lfoSyncRate);

    CHECK (rate.isVisible());
    CHECK_FALSE (syncRate.isVisible());
    CHECK (syncRate.getBounds() == rate.getBounds());
    auto& filter = findSwitch (editor, "12dB");

    setParameter (processor, Parameters::lfoSyncId, 1.0f);
    setParameter (processor, Parameters::filter24dbId, 1.0f);

    CHECK_FALSE (rate.isVisible());
    CHECK (syncRate.isVisible());
    CHECK (filter.getButtonText() == "24dB");

    setParameter (processor, Parameters::lfoSyncId, 0.0f);
    CHECK (rate.isVisible());
    CHECK_FALSE (syncRate.isVisible());
}
