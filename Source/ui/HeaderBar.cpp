#include "HeaderBar.h"
#include "CustomLookAndFeel.h"

HeaderBar::HeaderBar (const Title& titleText, const Layout& l, PresetManager& presets, PromptOverlay& o,
                      std::function<void()> m, const juce::String& about, juce::Component& meterToShow)
    : title (titleText), layout (l), prompt (o), showManual (std::move (m)), aboutText (about),
      presetActions (presets, prompt), presetBar (presets, presetActions), meter (meterToShow)
{
    // For screen readers, as the title is painted
    setTitle (title.bold + title.regular);

    menuButton.onClick = [this] { showMenu(); };

    addAndMakeVisible (presetBar);
    addAndMakeVisible (menuButton);
    addAndMakeVisible (meter);
}

void HeaderBar::paint (juce::Graphics& g)
{
    g.fillAll (findColour (backgroundColourId));

    // The meter's repaints reach here too
    const auto titleArea = getLocalBounds().withTrimmedLeft (layout.edge).withWidth (layout.titleWidth);
    if (! g.clipRegionIntersects (titleArea))
        return;

    auto text = titleText (title, layout.titleHeight);
    // Painted, so the colour comes from the LookAndFeel
    text.setColour (findColour (titleColourId));

    g.reduceClipRegion (titleArea);
    text.draw (g, titleArea.toFloat());
}

juce::AttributedString HeaderBar::titleText (const Title& title, float height)
{
    const auto font = [height] (float weight) { return juce::Font (CustomLookAndFeel::orbitron (weight).withHeight (height)); };

    juce::AttributedString text;
    text.setJustification (juce::Justification::centredLeft);
    text.setWordWrap (juce::AttributedString::none);
    text.append (title.bold, font (CustomLookAndFeel::boldWeight));
    text.append (title.regular, font (CustomLookAndFeel::regularWeight));
    return text;
}

void HeaderBar::resized()
{
    auto bounds = getLocalBounds().reduced (layout.edge, 0);
    meter.setBounds (bounds.removeFromRight (layout.meterWidth).reduced (0, layout.meterInset));

    auto controls = bounds.withTrimmedLeft (layout.titleWidth + layout.gap).withTrimmedRight (layout.gap);
    controls = controls.withSizeKeepingCentre (controls.getWidth(), layout.controlsHeight);
    menuButton.setBounds (controls.removeFromRight (28));
    controls.removeFromRight (4);
    presetBar.setBounds (controls);
}

void HeaderBar::showMenu()
{
    juce::PopupMenu menu;
    menu.setLookAndFeel (&getLookAndFeel());
    presetActions.addMenuItems (menu);
    menu.addSeparator();
    menu.addItem ("Manual", showManual);
    menu.addItem ("About", [this] { prompt.showMessage ("About", aboutText); });

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (menuButton));
}
