#include "ReeseSynth.h"
#include <algorithm>
#include "../shared/SynthSettings.h"

//==============================================================================
ReeseSynth::ReeseSynth()
{
    // Room for every MIDI note, so the audio thread never allocates
    heldNotes.reserve (128);
    setSettings (makeSynthSettings ([] (const ParameterSpecs::Spec& spec) { return spec.defaultValue; }));
}

void ReeseSynth::prepare (double sampleRate)
{
    voice.prepare (sampleRate);
}

void ReeseSynth::setSettings (const Settings& newSettings)
{
    settings = newSettings;
    voice.setParameters (settings.voice);
}

void ReeseSynth::process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const Transport& transport)
{
    buffer.clear();
    voice.setBpm (transport.bpm > 0.0 ? transport.bpm : Transport {}.bpm);

    const auto numSamples = buffer.getNumSamples();
    int currentSample = 0;

    for (const auto metadata : midi)
    {
        const auto position = std::clamp (metadata.samplePosition, currentSample, numSamples);

        if (position > currentSample)
        {
            voice.renderNextBlock (buffer, currentSample, position - currentSample);
            currentSample = position;
        }

        handleMidiEvent (metadata.getMessage());
    }

    voice.renderNextBlock (buffer, currentSample, numSamples - currentSample);

    const auto driveTrim = juce::jmap (settings.voice.drive, 1.0f, 0.75f);
    buffer.applyGain (settings.output * driveTrim);
}

void ReeseSynth::handleMidiEvent (const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        const auto note = message.getNoteNumber();
        removeHeldNote (note);
        heldNotes.push_back (note);

        if (heldNotes.size() == 1)
            voice.noteOn (note, message.getFloatVelocity());
        else
            voice.legatoTo (note);
    }
    else if (message.isNoteOff())
    {
        removeHeldNote (message.getNoteNumber());

        if (heldNotes.empty())
            voice.noteOff();
        else
            voice.legatoTo (heldNotes.back());
    }
    else if (message.isAllNotesOff() || message.isAllSoundOff())
    {
        heldNotes.clear();
        voice.noteOff();
    }
    else if (message.isPitchWheel())
    {
        voice.setPitchWheel (message.getPitchWheelValue());
    }
}

void ReeseSynth::removeHeldNote (int note)
{
    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
}
