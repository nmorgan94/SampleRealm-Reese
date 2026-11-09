#pragma once

#include "PresetActions.h"
#include "PresetBar.h"

//==============================================================================
/**
 * The strip along the top of the editor: the title, the preset bar, the menu and a meter.
 * Mirrored by webui/src/HeaderBar.tsx.
 */
class HeaderBar : public juce::Component
{
public:
    enum ColourIds
    {
        backgroundColourId = 0x5d10500,
        titleColourId
    };

    /** Bold then regular, as HeaderTitle in webui/src/HeaderBar.tsx. */
    struct Title
    {
        juce::String bold, regular;
    };

    /** Each plugin's sizes, as HeaderLayout in webui/src/HeaderBar.tsx. */
    struct Layout
    {
        int height;
        int edge;           // left of the title, right of the meter
        int titleWidth;
        float titleHeight;
        int gap;            // either side of the preset bar and menu
        int controlsHeight;
        int meterWidth;
        int meterInset;     // above and below the meter
    };

    HeaderBar (const Title&, const Layout&, PresetManager&, PromptOverlay&,
               std::function<void()> showManual, const juce::String& aboutText, juce::Component& meter);

    static juce::AttributedString titleText (const Title&, float height);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    const Title title;
    const Layout layout;
    PromptOverlay& prompt;
    const std::function<void()> showManual;
    const juce::String aboutText;

    PresetActions presetActions;
    PresetBar presetBar;
    juce::TextButton menuButton { "..." };
    juce::Component& meter;

    void showMenu();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderBar)
};
