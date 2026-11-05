#pragma once

#include "ParameterSpecs.h"
#include "../dsp/ReeseSynth.h"

//==============================================================================
template <typename ValueOf>
ReeseSynth::Settings makeSynthSettings (ValueOf&& valueOf)
{
    using namespace ParameterSpecs;

    // In range whoever calls: the voice indexes arrays with some of these
    const auto value = [&] (const Spec& spec) { return std::clamp (valueOf (spec), spec.min, spec.max); };
    const auto isOn = [&] (const Spec& spec) { return value (spec) >= 0.5f; };
    const auto index = [&] (const Spec& spec) { return juce::roundToInt (value (spec)); };

    ReeseSynth::Settings settings;
    auto& voice = settings.voice;
    voice.detune = value (detune);
    voice.sub = value (sub);
    voice.cutoff = value (cutoff);
    voice.resonance = value (resonance);
    voice.drive = value (drive);
    voice.lfoRate = value (lfoRate);
    voice.lfoDepth = value (lfoDepth);
    voice.lfoSync = isOn (lfoSync);
    voice.filter24db = isOn (filter24db);
    voice.lfoSyncRate = index (lfoSyncRate);
    voice.glideTime = value (glideTime);
    voice.unisonVoices = index (unisonVoices);
    voice.oscAWave = index (oscAWave);
    voice.oscBWave = index (oscBWave);
    voice.envelope = { value (attack), value (decay), value (sustain), value (release) };
    settings.output = value (output);
    return settings;
}
