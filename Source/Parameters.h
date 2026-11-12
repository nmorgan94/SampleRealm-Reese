#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "shared/ParameterSpecs.h"

//==============================================================================
namespace Parameters
{
    // Bump this when adding or restructuring parameters in a new plugin version
    constexpr int versionHint = 1;

    inline const juce::ParameterID outputId       { ParameterSpecs::output.id, versionHint };
    inline const juce::ParameterID detuneId       { ParameterSpecs::detune.id, versionHint };
    inline const juce::ParameterID subId          { ParameterSpecs::sub.id, versionHint };
    inline const juce::ParameterID cutoffId       { ParameterSpecs::cutoff.id, versionHint };
    inline const juce::ParameterID resonanceId    { ParameterSpecs::resonance.id, versionHint };
    inline const juce::ParameterID driveId        { ParameterSpecs::drive.id, versionHint };
    inline const juce::ParameterID lfoRateId      { ParameterSpecs::lfoRate.id, versionHint };
    inline const juce::ParameterID lfoDepthId     { ParameterSpecs::lfoDepth.id, versionHint };
    inline const juce::ParameterID lfoSyncId      { ParameterSpecs::lfoSync.id, versionHint };
    inline const juce::ParameterID filter24dbId   { ParameterSpecs::filter24db.id, versionHint };
    inline const juce::ParameterID lfoSyncRateId  { ParameterSpecs::lfoSyncRate.id, versionHint };
    inline const juce::ParameterID glideTimeId    { ParameterSpecs::glideTime.id, versionHint };
    inline const juce::ParameterID unisonVoicesId { ParameterSpecs::unisonVoices.id, versionHint };
    inline const juce::ParameterID oscAWaveId     { ParameterSpecs::oscAWave.id, versionHint };
    inline const juce::ParameterID oscBWaveId     { ParameterSpecs::oscBWave.id, versionHint };
    inline const juce::ParameterID attackId       { ParameterSpecs::attack.id, versionHint };
    inline const juce::ParameterID decayId        { ParameterSpecs::decay.id, versionHint };
    inline const juce::ParameterID sustainId      { ParameterSpecs::sustain.id, versionHint };
    inline const juce::ParameterID releaseId      { ParameterSpecs::release.id, versionHint };
}
