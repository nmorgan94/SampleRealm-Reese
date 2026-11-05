#pragma once

#include <array>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
/** The lowpass of JUCE's StateVariableTPTFilter, with two channels sharing one set of coefficients. */
class TptFilter
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
        update();
    }

    void reset() noexcept
    {
        s1.fill (0.0f);
        s2.fill (0.0f);
    }

    void setCutoffFrequency (float newCutoffHz)
    {
        cutoff = newCutoffHz;
        update();
    }

    void setResonance (float newResonance)
    {
        resonance = newResonance;
        update();
    }

    float processSample (int channel, float input) noexcept
    {
        auto& ls1 = s1[static_cast<size_t> (channel)];
        auto& ls2 = s2[static_cast<size_t> (channel)];

        const auto yHP = h * (input - ls1 * (g + R2) - ls2);

        const auto yBP = yHP * g + ls1;
        ls1 = yHP * g + yBP;

        const auto yLP = yBP * g + ls2;
        ls2 = yBP * g + yLP;

        return yLP;
    }

private:
    // As JUCE: tan in double, the coefficients in float
    void update()
    {
        g  = static_cast<float> (std::tan (juce::MathConstants<double>::pi * cutoff / sampleRate));
        R2 = static_cast<float> (1.0 / resonance);
        h  = static_cast<float> (1.0 / (1.0 + R2 * g + g * g));
    }

    double sampleRate = 44100.0;
    float cutoff = 1000.0f;
    float resonance = juce::MathConstants<float>::sqrt2 / 2.0f;
    float g = 0.0f, h = 0.0f, R2 = 0.0f;
    std::array<float, 2> s1 {}, s2 {};
};
