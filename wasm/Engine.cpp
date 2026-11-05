#include <juce_audio_basics/juce_audio_basics.h>
#include "shared/ParameterSpecs.h"
#include "shared/SynthSettings.h"
#include "shared/Transport.h"
#include "shared/UiInfoJson.h"
#include <BuildInfo.h>
#include "dsp/PeakMeter.h"
#include "dsp/ReeseSynth.h"

//==============================================================================
namespace
{
    constexpr int maxBlockSize = 128;

    constexpr auto numParams = static_cast<int> (ParameterSpecs::all.size());

    constexpr auto indexOf (const ParameterSpecs::Spec& spec)
    {
        return std::ranges::find (ParameterSpecs::all, &spec) - ParameterSpecs::all.begin();
    }

    auto defaultValues()
    {
        std::array<float, ParameterSpecs::all.size()> values;

        for (size_t i = 0; i < values.size(); ++i)
            values[i] = ParameterSpecs::all[i]->defaultValue;

        return values;
    }

    auto values = defaultValues();

    float valueOf (const ParameterSpecs::Spec& spec)
    {
        return values[static_cast<size_t> (indexOf (spec))];
    }

    ReeseSynth synth;
    PeakMeter meter { 1 };
    juce::AudioBuffer<float> buffer { 1, maxBlockSize };
    double currentSampleRate = 48000.0;
    Transport transport;

    // Played at the start of the next block
    juce::MidiBuffer pendingMidi;
}

#define EXPORT(name) extern "C" __attribute__ ((export_name (#name)))

EXPORT (uiInfo) const char* uiInfo()
{
    static const auto json = uiInfoJson (PLUGIN_VERSION, PLUGIN_GIT_COMMIT);
    return json.c_str();
}

EXPORT (prepare) void prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    synth.prepare (sampleRate);
}

EXPORT (setParam) void setParam (int index, float value)
{
    if (index >= 0 && index < numParams && ! std::isnan (value))
        values[static_cast<size_t> (index)] = value;
}

// The page plays the host: render moves the position on while playing
EXPORT (setTransport) void setTransport (double bpm, bool playing, double ppq)
{
    transport = { bpm, playing, ppq };
}

EXPORT (midi) void midi (int status, int data1, int data2)
{
    if (status < 0x80 || status >= 0xf0)
        return;

    pendingMidi.addEvent (juce::MidiMessage (status, data1 & 0x7f, data2 & 0x7f), 0);
}

// The output is mono: the page copies it to each channel
EXPORT (render) float* render (int numSamples)
{
    buffer.setSize (1, std::clamp (numSamples, 0, maxBlockSize), false, false, true);

    synth.setSettings (makeSynthSettings (valueOf));
    synth.process (buffer, pendingMidi, transport);
    pendingMidi.clear();
    meter.process (buffer);
    transport.advance (buffer.getNumSamples(), currentSampleRate);

    return buffer.getWritePointer (0);
}

// The loudest output sample since the last call, as linear gain
EXPORT (peak) float peak()
{
    return meter.readAndReset (0);
}
