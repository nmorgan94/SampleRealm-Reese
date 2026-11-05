#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include "ReeseVoice.h"
#include "../shared/Transport.h"

//==============================================================================
/** Mono, last-note priority: a note played while another is held glides to it. */
class ReeseSynth
{
public:
    struct Settings
    {
        ReeseVoice::Params voice;
        float output {};
    };

    ReeseSynth();

    void prepare (double sampleRate);
    void setSettings (const Settings& newSettings);

    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const Transport& transport);

private:
    void handleMidiEvent (const juce::MidiMessage& message);
    void removeHeldNote (int note);

    ReeseVoice voice;
    std::vector<int> heldNotes;
    Settings settings;
};
