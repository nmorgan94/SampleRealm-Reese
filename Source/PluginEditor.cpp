#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), adsrVisualizer(p.getAPVTS())
{
    setLookAndFeel(&customLookAndFeel);

    addAndMakeVisible(header);
    addAndMakeVisible(adsrVisualizer);

    configureSlider (controls[OSC_A_WAVE], "oscAWave", "OSC A");
    controls[OSC_A_WAVE].slider.setLookAndFeel(&triangleLookAndFeel);
    controls[OSC_A_WAVE].slider.setMouseDragSensitivity(75);
    
    configureSlider (controls[OSC_B_WAVE], "oscBWave", "OSC B");
    controls[OSC_B_WAVE].slider.setLookAndFeel(&triangleLookAndFeel);
    controls[OSC_B_WAVE].slider.setMouseDragSensitivity(75);
    configureSlider (controls[UNISON_VOICES], "unisonVoices", "UNISON");
    configureSlider (controls[DRIVE], "drive", "DRIVE");
    configureSlider (controls[DETUNE], "detune", "DETUNE");
    configureSlider (controls[SUB], "sub", "SUB");
    configureSlider (controls[CUTOFF], "cutoff", "CUTOFF");
    configureSlider (controls[RESONANCE], "resonance", "RESONANCE");
    configureSlider (controls[LFO_RATE], "lfoRate", "LFO RATE");

    lfoSyncButton.setButtonText("SYNC");
    lfoSyncButton.onClick = [this]() { updateLFORateControl(); };
    lfoSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "lfoSync", lfoSyncButton);
    addAndMakeVisible(lfoSyncButton);
    
    filter24dbButton.setButtonText("12dB");
    filter24dbButton.onClick = [this]()
    {
        filter24dbButton.setButtonText(filter24dbButton.getToggleState() ? "24dB" : "12dB");
    };
    filter24dbAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "filter24db", filter24dbButton);
    addAndMakeVisible(filter24dbButton);
    
    configureSlider (controls[LFO_DEPTH], "lfoDepth", "LFO DEPTH");
    configureSlider (controls[GLIDE_TIME], "glideTime", "GLIDE");
    configureSlider (controls[OUTPUT], "output", "OUTPUT");
    configureSlider (controls[ATTACK], "attack", "ATTACK");
    configureSlider (controls[DECAY], "decay", "DECAY");
    configureSlider (controls[SUSTAIN], "sustain", "SUSTAIN");
    configureSlider (controls[RELEASE], "release", "RELEASE");

    addAndMakeVisible(transportDisplay);

    prompt.setFont (CustomLookAndFeel::orbitronRegular().withHeight (14.0f));
    addChildComponent (prompt);
    addChildComponent (manual);

    startTimerHz (transportHz);

    setSize (width, height);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    stopTimer();

    controls[OSC_A_WAVE].slider.setLookAndFeel(nullptr);
    controls[OSC_B_WAVE].slider.setLookAndFeel(nullptr);
    
    setLookAndFeel(nullptr);
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    transportDisplay.setBPM(processorRef.getCurrentBPM());
}

void AudioPluginAudioProcessorEditor::updateSliderLabel (SliderWithAttachment& sliderControl, const juce::String& paramID)
{
    // Show waveform names for oscillator wave parameters
    if (paramID == "oscAWave" || paramID == "oscBWave")
    {
        const int waveValue = (int)sliderControl.slider.getValue();
        const char* waveNames[] = { "SQUARE", "SAW", "TRI" };
        sliderControl.label.setText(waveNames[waveValue], juce::dontSendNotification);
    }
    else if (paramID == "lfoSyncRate")
    {
        const int syncRateValue = (int)sliderControl.slider.getValue();
        const char* syncRateNames[] = {
            "1/16", "1/16T", "1/16D",
            "1/8", "1/8T", "1/8D",
            "1/4", "1/4T", "1/4D",
            "1/2", "1/2T", "1/2D",
            "1 BAR", "2 BARS", "4 BARS"
        };
        sliderControl.label.setText(syncRateNames[syncRateValue], juce::dontSendNotification);
    }
    else
    {
        int decimals = sliderControl.slider.getNumDecimalPlacesToDisplay();
        sliderControl.label.setText(juce::String(sliderControl.slider.getValue(), decimals), juce::dontSendNotification);
    }
}

void AudioPluginAudioProcessorEditor::updateLFORateControl()
{
    const bool syncEnabled = lfoSyncButton.getToggleState();
    
    if (syncEnabled)
    {
        configureSlider(controls[LFO_RATE], "lfoSyncRate", "LFO SYNC");
    }
    else
    {
        configureSlider(controls[LFO_RATE], "lfoRate", "LFO RATE");
    }
}

void AudioPluginAudioProcessorEditor::configureSlider (SliderWithAttachment& sliderControl,
                                                       const juce::String& paramID,
                                                       const juce::String& labelText)
{
    sliderControl.originalLabelText = labelText;
    
    sliderControl.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    sliderControl.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    sliderControl.slider.setMouseDragSensitivity(150);
    
    // Set up callbacks to track active slider
    sliderControl.slider.onDragStart = [this, &sliderControl, paramID]()
    {
        activeSlider = &sliderControl.slider;
        updateSliderLabel(sliderControl, paramID);
    };
    
    sliderControl.slider.onDragEnd = [this, &sliderControl]()
    {
        activeSlider = nullptr;
        sliderControl.label.setText(sliderControl.originalLabelText, juce::dontSendNotification);
    };
    
    sliderControl.slider.onValueChange = [this, &sliderControl, paramID]()
    {
        if (activeSlider == &sliderControl.slider)
            updateSliderLabel(sliderControl, paramID);
    };
    
    addAndMakeVisible (sliderControl.slider);

    sliderControl.label.setText (labelText, juce::dontSendNotification);
    sliderControl.label.setJustificationType (juce::Justification::centred);
    sliderControl.label.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    sliderControl.label.attachToComponent (&sliderControl.slider, false);
    addAndMakeVisible (sliderControl.label);

    sliderControl.attachment = std::make_unique<SliderAttachment> (processorRef.getAPVTS(), paramID, sliderControl.slider);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient bgGradient(juce::Colour(0xff0a0e1a), 0, 0,
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
    auto lfoDepthBounds = controls[LFO_DEPTH].slider.getBounds().toFloat();
    auto attackBounds = controls[ATTACK].slider.getBounds().toFloat();
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

    for (int i = 0; i < (int) controls.size(); ++i)
    {
        const auto row = i / columns;
        const auto column = i % columns;

        auto cell = juce::Rectangle<int> (area.getX() + (column * columnWidth),
                                          area.getY() + (row * rowHeight),
                                          columnWidth,
                                          rowHeight);

        controls[(size_t) i].label.setBounds (cell.removeFromTop (24));
        controls[(size_t) i].slider.setBounds (cell);
    }
    
    auto lfoRateSliderBounds = controls[LFO_RATE].slider.getBounds();
    auto syncButtonBounds = juce::Rectangle<int>(
        lfoRateSliderBounds.getX() + 40,
        lfoRateSliderBounds.getY() + 5,
        30,
        10
    );
    lfoSyncButton.setBounds(syncButtonBounds);
    
    auto cutoffSliderBounds = controls[CUTOFF].slider.getBounds();
    auto filter24dbButtonBounds = juce::Rectangle<int>(
        cutoffSliderBounds.getX() + 40,
        cutoffSliderBounds.getY() + 5,
        30,
        10
    );
    filter24dbButton.setBounds(filter24dbButtonBounds);
}
