#include "ReeseVoice.h"

//==============================================================================
namespace
{
constexpr auto twoPi = juce::MathConstants<float>::twoPi;

// Waveform generation functions
float makeBipolarSaw (float phase)
{
    return (2.0f * phase) - 1.0f;
}

float makeBipolarSquare (float phase)
{
    return (phase < 0.5f) ? -1.0f : 1.0f;
}

float makeBipolarTriangle (float phase)
{
    return (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
}

float generateWaveform (float phase, int waveType)
{
    switch (waveType)
    {
        case 0: return makeBipolarSquare (phase);
        case 1: return makeBipolarSaw (phase);
        case 2: return makeBipolarTriangle (phase);
        default: return makeBipolarSaw (phase);
    }
}

// Two octaves down
float frequencyOf (int midiNoteNumber)
{
    return (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber - 24);
}
}

//==============================================================================
ReeseVoice::ReeseVoice()
{
    setPitchWheel (0x2000);
}

void ReeseVoice::setParameters (const Params& newParams)
{
    params = newParams;
    filter.setResonance (params.resonance);
}

void ReeseVoice::noteOn (int midiNoteNumber, float velocity)
{
    level = velocity;
    targetFrequencyHz = frequencyOf (midiNoteNumber);
    currentFrequencyHz = targetFrequencyHz;
    ampEnvelope.noteOn();
}

void ReeseVoice::legatoTo (int midiNoteNumber)
{
    // renderSample glides there
    targetFrequencyHz = frequencyOf (midiNoteNumber);
}

void ReeseVoice::noteOff()
{
    ampEnvelope.noteOff();
}

void ReeseVoice::setPitchWheel (int value)
{
    pitchBendSemitones = juce::jmap ((float) value, 0.0f, 16383.0f, -2.0f, 2.0f);
}

void ReeseVoice::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    ampEnvelope.setSampleRate (sampleRate);

    filter.prepare (sampleRate);
}

float ReeseVoice::getLFORate() const
{
    if (! params.lfoSync)
        return params.lfoRate;

    // Beats per cycle for each timing option
    // T = Triplet (2/3 of normal), D = Dotted (1.5x normal)
    const float beatsPerCycle[] = {
        0.25f,                  // 1/16
        0.25f * 2.0f / 3.0f,    // 1/16T
        0.25f * 1.5f,           // 1/16D
        0.5f,                   // 1/8
        0.5f * 2.0f / 3.0f,     // 1/8T
        0.5f * 1.5f,            // 1/8D
        1.0f,                   // 1/4
        1.0f * 2.0f / 3.0f,     // 1/4T
        1.0f * 1.5f,            // 1/4D
        2.0f,                   // 1/2
        2.0f * 2.0f / 3.0f,     // 1/2T
        2.0f * 1.5f,            // 1/2D
        4.0f,                   // 1 Bar
        8.0f,                   // 2 Bars
        16.0f                   // 4 Bars
    };
    const float beats = beatsPerCycle[params.lfoSyncRate];

    return static_cast<float>((bpm / 60.0) / beats);
}

float ReeseVoice::renderSample()
{
    // Only legatoTo leaves a gap to glide across
    if (std::abs (currentFrequencyHz - targetFrequencyHz) > 0.01f)
        currentFrequencyHz += (targetFrequencyHz - currentFrequencyHz) * glideCoeff;
    else
        currentFrequencyHz = targetFrequencyHz;

    const auto frequencyA = currentFrequencyHz * std::pow (2.0f, pitchBendSemitones / 12.0f);
    const auto frequencyB = frequencyA * std::pow (2.0f, (params.detune * 0.5f) / 1200.0f);
    const auto subFrequency = 0.5f * frequencyA;
    const auto sampleRateFloat = (float) currentSampleRate;
    
    const auto unisonVoices = static_cast<size_t>(params.unisonVoices);
    const int oscAWaveType = params.oscAWave;
    const int oscBWaveType = params.oscBWave;
    
    // Render unison oscillators
    float unisonSum = 0.0f;
    
    for (size_t i = 0; i < unisonVoices; ++i)
    {
        // Calculate detune offset for this voice (-50% to +50% of detune range)
        const float voiceDetune = (unisonVoices > 1)
            ? ((float) i / (float) (unisonVoices - 1) - 0.5f) * 2.0f
            : 0.0f;
        
        const auto spread = std::pow (2.0f, voiceDetune * params.detune / 1200.0f);
        const auto unisonFreqA = frequencyA * spread;
        const auto unisonFreqB = frequencyB * spread;
        
        // Update phases for this unison voice
        unisonPhasesA[i] += unisonFreqA / sampleRateFloat;
        unisonPhasesB[i] += unisonFreqB / sampleRateFloat;
        
        unisonPhasesA[i] -= std::floor (unisonPhasesA[i]);
        unisonPhasesB[i] -= std::floor (unisonPhasesB[i]);
        
        const auto waveA = generateWaveform (unisonPhasesA[i], oscAWaveType);
        const auto waveB = generateWaveform (unisonPhasesB[i], oscBWaveType);
        
        unisonSum += (waveA + waveB) * 0.5f;
    }
    
    // Normalize by number of voices to prevent volume increase
    unisonSum /= (float) unisonVoices;

    subPhase += subFrequency / sampleRateFloat;
    lfoPhase += getLFORate() / sampleRateFloat;

    subPhase -= std::floor (subPhase);
    lfoPhase -= std::floor (lfoPhase);

    const auto sub = std::sin (twoPi * subPhase) * params.sub;
    const auto lfo = std::sin (twoPi * lfoPhase) * params.lfoDepth;
    const auto dry = unisonSum + sub;
    const auto driven = std::tanh (dry * (1.0f + params.drive * 6.0f));
    const auto envelope = ampEnvelope.getNextSample();

    const auto modulatedCutoff = juce::jlimit (40.0f, 18000.0f,
                                               params.cutoff * (1.0f + (0.35f * lfo)));
    filter.setCutoffFrequency (modulatedCutoff);
    
    auto filtered = filter.processSample (0, driven * envelope * level);
    
    // Apply second stage if 24dB mode is enabled
    if (params.filter24db)
        filtered = filter.processSample (1, filtered);
    
    return filtered;
}

void ReeseVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! ampEnvelope.isActive())
        return;

    // Not in setParameters: during a release this resets the release rate, as before
    ampEnvelope.setParameters (params.envelope);

    // Exponential smoothing
    glideCoeff = 1.0f - std::exp (-1.0f / (params.glideTime * (float) currentSampleRate));

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto value = renderSample();

        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
            outputBuffer.addSample (channel, startSample + sample, value);

        if (! ampEnvelope.isActive())
            break;
    }
}
