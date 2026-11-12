#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), adsrVisualizer(p.getAPVTS())
{
    using namespace ParameterSpecs;

    setLookAndFeel(&customLookAndFeel);

    addAndMakeVisible(header);
    addAndMakeVisible(adsrVisualizer);

    configureKnob (knobs[OSC_A_WAVE], oscAWave, "OSC A");
    configureKnob (knobs[OSC_B_WAVE], oscBWave, "OSC B");
    configureKnob (knobs[SUB], sub, "SUB");
    configureKnob (knobs[DETUNE], detune, "DETUNE");
    configureKnob (knobs[GLIDE_TIME], glideTime, "GLIDE");
    configureKnob (knobs[UNISON_VOICES], unisonVoices, "UNISON");
    configureKnob (knobs[DRIVE], drive, "DRIVE");
    configureKnob (knobs[OUTPUT], output, "OUTPUT");
    configureKnob (knobs[CUTOFF], cutoff, "CUTOFF");
    configureKnob (knobs[RESONANCE], resonance, "RESONANCE");
    configureKnob (knobs[LFO_RATE], lfoRate, "LFO RATE");
    configureKnob (lfoSyncRateKnob, lfoSyncRate, "LFO SYNC");
    configureKnob (knobs[LFO_DEPTH], lfoDepth, "LFO DEPTH");
    configureKnob (knobs[ATTACK], attack, "ATTACK");
    configureKnob (knobs[DECAY], decay, "DECAY");
    configureKnob (knobs[SUSTAIN], sustain, "SUSTAIN");
    configureKnob (knobs[RELEASE], release, "RELEASE");

    lfoSyncButton.setButtonText("SYNC");
    lfoSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), lfoSync.id, lfoSyncButton);
    addAndMakeVisible(lfoSyncButton);

    filter24dbAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), filter24db.id, filter24dbButton);
    addAndMakeVisible(filter24dbButton);

    // Clicked by the attachment too, when the host or a preset sets the parameter
    lfoSyncButton.onClick = [this] { updateFromSwitches(); };
    filter24dbButton.onClick = [this] { updateFromSwitches(); };
    updateFromSwitches();

    addAndMakeVisible(transportDisplay);

    addChildComponent (prompt);
    addChildComponent (manual);

    startTimerHz (transportHz);

    setSize (width, height);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    stopTimer();

    knobs[OSC_A_WAVE].getSlider().setLookAndFeel(nullptr);
    knobs[OSC_B_WAVE].getSlider().setLookAndFeel(nullptr);

    setLookAndFeel(nullptr);
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    transportDisplay.setBPM(processorRef.getCurrentBPM());
}

void AudioPluginAudioProcessorEditor::updateFromSwitches()
{
    const auto synced = lfoSyncButton.getToggleState();
    knobs[LFO_RATE].setVisible (! synced);
    lfoSyncRateKnob.setVisible (synced);

    filter24dbButton.setButtonText (filter24dbButton.getToggleState() ? "24dB" : "12dB");
}

void AudioPluginAudioProcessorEditor::configureKnob (LabeledSlider& knob, const ParameterSpecs::Spec& spec, const juce::String& labelText)
{
    const auto isWave = &spec == &ParameterSpecs::oscAWave || &spec == &ParameterSpecs::oscBWave;

    // The id lets tests find the knob
    knob.setComponentID (spec.id);
    knob.setLabelText (labelText);
    knob.setNamePlacement (LabeledSlider::NamePlacement::above);
    knob.setNameHeight (14.0f);
    addAndMakeVisible (knob);

    auto& slider = knob.getSlider();
    slider.setMouseDragSensitivity (isWave ? 75 : 150);

    if (isWave)
        slider.setLookAndFeel (&triangleLookAndFeel);

    attachments.push_back (std::make_unique<SliderAttachment> (processorRef.getAPVTS(), spec.id, slider));

    // After the attachment, which sets the parameter's own text. As dragText in webui/src/labels.ts
    if (spec.kind == ParameterSpecs::Kind::choice)
    {
        slider.textFromValueFunction = [parameterText = slider.textFromValueFunction] (double value)
        {
            return parameterText (value).toUpperCase();
        };
    }
}

juce::Rectangle<int> AudioPluginAudioProcessorEditor::knobArea (ControlIndex index) const
{
    return knobs[index].getBounds().withTrimmedTop (LabeledSlider::labelHeight);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient bgGradient(juce::Colour(CustomLookAndFeel::BACKGROUND_TOP), 0, 0,
                                    juce::Colour(0xff151925), 0, (float)getHeight(),
                                    false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    auto bounds = getLocalBounds().reduced (3);
    bounds.removeFromTop (headerLayout.height);

    auto panelBounds = bounds.toFloat();

    // Main panel with depth

    // Outer glow
    g.setColour(juce::Colour(0xff00d9ff).withAlpha(0.08f));
    g.fillRoundedRectangle(panelBounds.expanded(2.0f), 14.0f);

    // Panel background with gradient
    juce::ColourGradient panelGradient(juce::Colour(0xff1a1f2e).withAlpha(0.6f),
                                       panelBounds.getX(), panelBounds.getY(),
                                       juce::Colour(0xff0f1419).withAlpha(0.8f),
                                       panelBounds.getX(), panelBounds.getBottom(),
                                       false);
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(panelBounds, 12.0f);

    // Panel border with gradient
    juce::ColourGradient borderGradient(juce::Colour(0xff2a3f5f),
                                        panelBounds.getX(), panelBounds.getY(),
                                        juce::Colour(0xff1a2f4f),
                                        panelBounds.getX(), panelBounds.getBottom(),
                                        false);
    g.setGradientFill(borderGradient);
    g.drawRoundedRectangle(panelBounds, 12.0f, 2.0f);

    // Inner highlight
    g.setColour(juce::Colour(0xff3a4f6f).withAlpha(0.3f));
    g.drawRoundedRectangle(panelBounds.reduced(2.0f), 10.0f, 1.0f);

    // Section dividers
    auto dividerArea = panelBounds.reduced(20.0f, 15.0f);
    auto dividerY = dividerArea.getCentreY();

    // Divider glow
    g.setColour(juce::Colour(0xff00d9ff).withAlpha(0.1f));
    g.drawLine(dividerArea.getX(), dividerY, dividerArea.getRight(), dividerY, 2.0f);

    // Divider line
    g.setColour(juce::Colour(0xff2a3f5f).withAlpha(0.5f));
    g.drawLine(dividerArea.getX(), dividerY, dividerArea.getRight(), dividerY, 1.0f);
    
    // Vertical divider between LFO DEPTH and ATTACK sliders
    auto lfoDepthBounds = knobArea (LFO_DEPTH).toFloat();
    auto attackBounds = knobArea (ATTACK).toFloat();
    auto verticalDividerX = (lfoDepthBounds.getRight() + attackBounds.getX()) / 2.0f;

    g.setColour(juce::Colour(0xff00d9ff).withAlpha(0.1f));
    g.drawLine(verticalDividerX, lfoDepthBounds.getY() + 15.0f, verticalDividerX, lfoDepthBounds.getBottom() - 15.0f, 2.0f);

    g.setColour(juce::Colour(0xff2a3f5f).withAlpha(0.5f));
    g.drawLine(verticalDividerX, lfoDepthBounds.getY() + 15.0f, verticalDividerX, lfoDepthBounds.getBottom() - 15.0f, 1.0f);

    // Corner accents
    auto drawCornerAccent = [&](float x, float y, bool flipX, bool flipY)
    {
        juce::Path accent;
        accent.startNewSubPath(0, 15);
        accent.lineTo(0, 0);
        accent.lineTo(15, 0);
        
        auto transform = juce::AffineTransform::translation(x, y);
        if (flipX) transform = transform.scaled(-1, 1, x, y);
        if (flipY) transform = transform.scaled(1, -1, x, y);
        
        accent.applyTransform(transform);
        
        g.setColour(juce::Colour(0xff00d9ff).withAlpha(0.4f));
        g.strokePath(accent, juce::PathStrokeType(2.0f));
    };
    
    auto cornerInset = 8.0f;
    drawCornerAccent(panelBounds.getX() + cornerInset, panelBounds.getY() + cornerInset, false, false);
    drawCornerAccent(panelBounds.getRight() - cornerInset, panelBounds.getY() + cornerInset, true, false);
    drawCornerAccent(panelBounds.getX() + cornerInset, panelBounds.getBottom() - cornerInset, false, true);
    drawCornerAccent(panelBounds.getRight() - cornerInset, panelBounds.getBottom() - cornerInset, true, true);
    
    // Version text in bottom-right corner
    g.setFont(CustomLookAndFeel::orbitronRegular().withPointHeight(9.0f));
    g.setColour(juce::Colour(0xff00d9ff).withAlpha(0.4f));
    auto versionArea = juce::Rectangle<int>(bounds.getRight() - 60, bounds.getBottom() - 23, 50, 12);
    g.drawText("v" + juce::String(JucePlugin_VersionString), versionArea, juce::Justification::centredRight);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (3);
    header.setBounds (bounds.removeFromTop (headerLayout.height));

    auto panelArea = bounds;
    auto transportArea = juce::Rectangle<int>(panelArea.getX() + 15, panelArea.getBottom() - 23, 140, 12);
    transportDisplay.setBounds(transportArea);
    auto area = panelArea.reduced (12);

    constexpr int columns = 8;
    constexpr int rows = 2;
    const auto rowHeight = area.getHeight() / rows;
    const auto columnWidth = area.getWidth() / columns;

    auto adsrRowBounds = juce::Rectangle<int>(area.getX() + (4 * columnWidth),
                                               area.getY() + (1 * rowHeight) + 10,
                                               4 * columnWidth,
                                               rowHeight - 15);
    adsrVisualizer.setBounds(adsrRowBounds);

    int cell = 0;

    for (auto& knob : knobs)
    {
        const auto row = cell / columns;
        const auto column = cell % columns;

        knob.setBounds (area.getX() + (column * columnWidth),
                        area.getY() + (row * rowHeight),
                        columnWidth,
                        rowHeight);
        ++cell;
    }

    lfoSyncRateKnob.setBounds (knobs[LFO_RATE].getBounds());

    auto lfoRateSliderBounds = knobArea (LFO_RATE);
    auto syncButtonBounds = juce::Rectangle<int>(
        lfoRateSliderBounds.getX() + 40,
        lfoRateSliderBounds.getY() + 5,
        30,
        10
    );
    lfoSyncButton.setBounds(syncButtonBounds);
    
    auto cutoffSliderBounds = knobArea (CUTOFF);
    auto filter24dbButtonBounds = juce::Rectangle<int>(
        cutoffSliderBounds.getX() + 40,
        cutoffSliderBounds.getY() + 5,
        30,
        10
    );
    filter24dbButton.setBounds(filter24dbButtonBounds);
}
