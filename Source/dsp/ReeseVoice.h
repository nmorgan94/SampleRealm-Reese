#pragma once

#include <array>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
class ReeseVoice
{
public:
    struct Params
    {
        float detune {}, sub {}, cutoff {}, resonance {}, drive {}, lfoRate {}, lfoDepth {};
        bool lfoSync {}, filter24db {};
        int lfoSyncRate {};
        float glideTime {};
        int unisonVoices {}, oscAWave {}, oscBWave {};
        juce::ADSR::Parameters envelope;
    };

    ReeseVoice();

    void setParameters (const Params& newParams);
    void setBpm (double newBpm) { bpm = newBpm; }

    void prepare (double sampleRate);

    /** Starts the envelope from where it is, at this pitch. */
    void noteOn (int midiNoteNumber, float velocity);
    /** Glides to this pitch, keeping the envelope and level. */
    void legatoTo (int midiNoteNumber);
    void noteOff();
    /** 0..16383 */
    void setPitchWheel (int value);

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples);

private:
    float renderSample();
    float getLFORate() const;

    Params params;
    double bpm {};

    juce::ADSR ampEnvelope;
    // Channel 1 is the second stage of the 24 dB slope
    juce::dsp::StateVariableTPTFilter<float> filter;
    double currentSampleRate { 44100.0 };
    float level { 0.0f };
    float targetFrequencyHz { 110.0f };
    float currentFrequencyHz { 0.0f };
    float glideCoeff { 1.0f };
    float pitchBendSemitones { 0.0f };
    float subPhase { 0.0f };
    float lfoPhase { 0.0f };

    std::array<float, 8> unisonPhasesA {};
    std::array<float, 8> unisonPhasesB {};
};
