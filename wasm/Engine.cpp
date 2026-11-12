#include <juce_audio_basics/juce_audio_basics.h>
#include <samplerealm/BuildCommit.h>
#include <samplerealm/ParameterValues.h>
#include <samplerealm/PeakMeter.h>
#include <samplerealm/Transport.h>
#include <samplerealm/UiInfoJson.h>
#include <samplerealm/WasmExport.h>
#include "shared/ParameterSpecs.h"
#include "shared/SynthSettings.h"
#include "dsp/ReeseSynth.h"

//==============================================================================
namespace
{
    constexpr int maxBlockSize = 128;

    ParameterValues values { ParameterSpecs::all };

    ReeseSynth synth;
    PeakMeter meter { 1 };
    // Mono: the page copies the one channel to each of its outputs
    juce::AudioBuffer<float> buffer { 1, maxBlockSize };
    double currentSampleRate = 48000.0;
    Transport transport;

    // Played at the start of the next block
    juce::MidiBuffer pendingMidi;
}

WASM_EXPORT (uiInfo) const char* uiInfo()
{
    static const auto json = uiInfoJson (ParameterSpecs::all, PLUGIN_VERSION, buildCommit);
    return json.c_str();
}

// Always one channel, whatever the page asks for
WASM_EXPORT (prepare) void prepare (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    synth.prepare (sampleRate);
}

WASM_EXPORT (setParam) void setParam (int index, float value)
{
    values.set (index, value);
}

// The page plays the host: render moves the position on while playing
WASM_EXPORT (setTransport) void setTransport (double bpm, bool playing, double ppq)
{
    transport = { bpm, playing, ppq };
}

WASM_EXPORT (midi) void midi (int status, int data1, int data2)
{
    if (status < 0x80 || status >= 0xf0)
        return;

    pendingMidi.addEvent (juce::MidiMessage (status, data1 & 0x7f, data2 & 0x7f), 0);
}

// The output is read back from here after render, for any channel the page asks for
WASM_EXPORT (channel) float* channel (int)
{
    return buffer.getWritePointer (0);
}

WASM_EXPORT (render) void render (int numSamples)
{
    juce::AudioBuffer<float> block { buffer.getArrayOfWritePointers(), 1, std::clamp (numSamples, 0, maxBlockSize) };

    synth.setSettings (makeSynthSettings (values));
    synth.process (block, pendingMidi, transport);
    pendingMidi.clear();
    meter.process (block);
    transport.advance (block.getNumSamples(), currentSampleRate);
}

// The loudest output sample since the last call, as linear gain
WASM_EXPORT (peak) float peak (int index)
{
    return index == 0 ? meter.readAndReset (0) : 0.0f;
}
