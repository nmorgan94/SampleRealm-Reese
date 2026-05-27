#include "Parameters.h"

namespace Parameters
{
    static constexpr int versionHint = 1;

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "output", versionHint }, "Output",
                                                                        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.75f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "detune", versionHint }, "Detune",
                                                                        juce::NormalisableRange<float> (0.0f, 40.0f, 0.01f), 12.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "sub", versionHint }, "Sub",
                                                                        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "cutoff", versionHint }, "Cutoff",
                                                                        juce::NormalisableRange<float> (40.0f, 18000.0f, 0.01f, 0.35f), 1800.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "resonance", versionHint }, "Resonance",
                                                                        juce::NormalisableRange<float> (0.1f, 1.2f, 0.001f), 0.2f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "drive", versionHint }, "Drive",
                                                                        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.15f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "lfoRate", versionHint }, "LFO Rate",
                                                                        juce::NormalisableRange<float> (0.05f, 20.0f, 0.001f, 0.35f), 0.8f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "lfoDepth", versionHint }, "LFO Depth",
                                                                        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.25f));
        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "lfoSync", versionHint }, "LFO Sync", false));
        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "filter24db", versionHint }, "Filter 24dB", false));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "lfoSyncRate", versionHint }, "LFO Sync Rate",
                                                                         juce::StringArray {
                                                                             "1/16", "1/16T", "1/16D",
                                                                             "1/8", "1/8T", "1/8D",
                                                                             "1/4", "1/4T", "1/4D",
                                                                             "1/2", "1/2T", "1/2D",
                                                                             "1 Bar", "2 Bars", "4 Bars"
                                                                         }, 6));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "glideTime", versionHint }, "Glide Time",
                                                                        juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f, 0.5f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "unisonVoices", versionHint }, "Unison Voices",
                                                                      1, 8, 1));
        params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "oscAWave", versionHint }, "Osc A Wave",
                                                                      0, 2, 1));
        params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "oscBWave", versionHint }, "Osc B Wave",
                                                                      0, 2, 1));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "attack", versionHint }, "Attack",
                                                                        juce::NormalisableRange<float> (0.001f, 0.1f, 0.001f, 0.5f), 0.01f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "decay", versionHint }, "Decay",
                                                                        juce::NormalisableRange<float> (0.01f, 1.0f, 0.001f, 0.4f), 0.08f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "sustain", versionHint }, "Sustain",
                                                                        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.85f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "release", versionHint }, "Release",
                                                                        juce::NormalisableRange<float> (0.01f, 2.0f, 0.001f, 0.4f), 0.2f));
        return { params.begin(), params.end() };
    }
}
