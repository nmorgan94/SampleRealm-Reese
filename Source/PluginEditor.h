#pragma once

#include "PluginProcessor.h"
#include "ui/CustomLookAndFeel.h"
#include "ui/TriangleSelectorLookAndFeel.h"
#include "ui/ADSRVisualizer.h"
#include "ui/TransportDisplay.h"
#include <BinaryData.h>

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

    void configureKnob (LabeledSlider& knob, const ParameterSpecs::Spec& spec, const juce::String& labelText);

    // The LFO knob and the filter button follow their switches, however they were set
    void updateFromSwitches();

    // A knob's own area, under its name
    juce::Rectangle<int> knobArea (ControlIndex index) const;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    CustomLookAndFeel customLookAndFeel;
    TriangleSelectorLookAndFeel triangleLookAndFeel;
    PromptOverlay prompt;
    ManualOverlay manual { juce::String::createStringFromData (BinaryData::Manual_md, BinaryData::Manual_mdSize) };
    LevelMeter outputMeter { processorRef.getOutputMeter() };
    HeaderBar header { title, headerLayout, processorRef.getPresetManager(), prompt,
                       [this] { manual.show(); }, About::text (processorRef), outputMeter };

    std::array<LabeledSlider, 16> knobs;
    // In LFO RATE's place while the LFO is synced
    LabeledSlider lfoSyncRateKnob;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;

    ADSRVisualizer adsrVisualizer;
    TransportDisplay transportDisplay;
    
    juce::ToggleButton lfoSyncButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lfoSyncAttachment;
    
    juce::ToggleButton filter24dbButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> filter24dbAttachment;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
