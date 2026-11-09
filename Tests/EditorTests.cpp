#include <catch2/catch_test_macros.hpp>

#include "TestHelpers.h"

using namespace TestHelpers;
using Editor = AudioPluginAudioProcessorEditor;

template <typename T>
static std::vector<T*> descendantsOf (juce::Component& parent)
{
    std::vector<T*> found;

    for (auto* child : parent.getChildren())
    {
        if (auto* match = dynamic_cast<T*> (child))
            found.push_back (match);

        const auto below = descendantsOf<T> (*child);
        found.insert (found.end(), below.begin(), below.end());
    }

    return found;
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
    CHECK (lines[2] == juce::String ("Commit ") + About::commit);
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
}

TEST_CASE_METHOD (ProcessorFixture, "The header's title fits its width and leaves the preset name room", "[editor]")
{
    juce::TextLayout layout;
    layout.createLayout (HeaderBar::titleText (Editor::title, Editor::headerLayout.titleHeight), 10000.0f);
    REQUIRE (layout.getNumLines() == 1);

    const auto titleWidth = layout.getLine (0).getLineBounds().getRight();
    CAPTURE (titleWidth);
    CHECK (titleWidth <= static_cast<float> (Editor::headerLayout.titleWidth));
    CHECK (titleWidth > static_cast<float> (Editor::headerLayout.titleWidth - 8));

    Editor editor (processor);
    juce::TextButton* name = nullptr;

    for (auto* button : descendantsOf<juce::TextButton> (editor))
        if (button->getButtonText() == "Init")
            name = button;

    REQUIRE (name != nullptr);
    CAPTURE (name->getWidth());
    CHECK (name->getWidth() >= 100);
}
