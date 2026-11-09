#pragma once

#include "../PluginProcessor.h"

//==============================================================================
class WebViewEditor final : public juce::AudioProcessorEditor,
                            private juce::Timer
{
public:
    explicit WebViewEditor (AudioPluginAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    juce::WebBrowserComponent::Options createOptions();
    juce::WebBrowserComponent::Options withPresetFunctions (const juce::WebBrowserComponent::Options&);
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    AudioPluginAudioProcessor& processorRef;

    // Declaration order matters: relays, browser, attachments
    std::vector<std::unique_ptr<juce::WebSliderRelay>> relays;
    std::unique_ptr<juce::WebBrowserComponent> browser;
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>> attachments;

    juce::uint32 lastPresetChange = 0;

    juce::MemoryInputStream uiZipStream;
    juce::ZipFile uiZip { uiZipStream };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebViewEditor)
};
