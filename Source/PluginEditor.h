#pragma once

#include "PluginProcessor.h"
#include "ui/About.h"
#include "ui/CustomLookAndFeel.h"
#include "ui/TriangleSelectorLookAndFeel.h"
#include "ui/HeaderBar.h"
#include "ui/LevelMeter.h"
#include "ui/Manual.h"
#include "ui/PromptOverlay.h"
#include "ui/ADSRVisualizer.h"
#include "ui/TransportDisplay.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               private juce::Timer
{
public:
    static constexpr int width = 625;
    static constexpr int height = 300;
    static constexpr int transportHz = 33;

    // As HEADER_TITLE and HEADER_LAYOUT in webui/src/ReeseEditor.tsx
    inline static const HeaderBar::Title title { "SampleRealm: ", "REESE" };
    static constexpr HeaderBar::Layout headerLayout { .height = 40, .edge = 0, .titleWidth = 340, .titleHeight = 35.112f,
                                                      .gap = 8, .controlsHeight = 24, .meterWidth = LevelMeter::barWidth, .meterInset = 4 };

    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    enum ControlIndex
    {
        OSC_A_WAVE = 0,
        OSC_B_WAVE,
        SUB,
        DETUNE,
        GLIDE_TIME,
        UNISON_VOICES,
        DRIVE,
        OUTPUT,
        CUTOFF,
        RESONANCE,
        LFO_RATE,
        LFO_DEPTH,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE
    };

    struct SliderWithAttachment
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
        juce::String originalLabelText;
    };

    void configureSlider (SliderWithAttachment& sliderControl, const juce::String& paramID, const juce::String& labelText);
    void updateSliderLabel (SliderWithAttachment& sliderControl, const juce::String& paramID);
    void updateLFORateControl();

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    CustomLookAndFeel customLookAndFeel;
    TriangleSelectorLookAndFeel triangleLookAndFeel;
    PromptOverlay prompt;
    ManualOverlay manual;
    LevelMeter outputMeter { processorRef.getOutputMeter() };
    HeaderBar header { title, headerLayout, processorRef.getPresetManager(), prompt,
                       [this] { manual.show(); }, About::text (processorRef), outputMeter };

    std::array<SliderWithAttachment, 16> controls;
    juce::Slider* activeSlider = nullptr;
    
    ADSRVisualizer adsrVisualizer;
    TransportDisplay transportDisplay;
    
    juce::ToggleButton lfoSyncButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lfoSyncAttachment;
    
    juce::ToggleButton filter24dbButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> filter24dbAttachment;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
