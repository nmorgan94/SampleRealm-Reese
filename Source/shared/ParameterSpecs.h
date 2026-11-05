#pragma once

#include <algorithm>
#include <array>
#include <span>

//==============================================================================
namespace ParameterSpecs
{
    enum class Kind { real, integer, choice };

    struct Spec
    {
        const char* id;
        const char* name;
        Kind kind;
        float min, max, defaultValue;
        float step = 0.0f;
        float skew = 1.0f;
        const char* unit = "";
        std::span<const char* const> choices = {};
    };

    constexpr Spec integer (const char* id, const char* name, int min, int max, int defaultValue, const char* unit = "")
    {
        return { id, name, Kind::integer, static_cast<float> (min), static_cast<float> (max),
                 static_cast<float> (defaultValue), 1.0f, 1.0f, unit };
    }

    constexpr Spec choice (const char* id, const char* name, std::span<const char* const> choices, int defaultValue)
    {
        return { id, name, Kind::choice, 0.0f, static_cast<float> (choices.size() - 1),
                 static_cast<float> (defaultValue), 1.0f, 1.0f, "", choices };
    }

    inline constexpr const char* offOn[] { "Off", "On" };

    inline constexpr const char* syncRates[] {
        "1/16", "1/16T", "1/16D",
        "1/8", "1/8T", "1/8D",
        "1/4", "1/4T", "1/4D",
        "1/2", "1/2T", "1/2D",
        "1 Bar", "2 Bars", "4 Bars"
    };

    inline constexpr Spec output       { "output",    "Output",    Kind::real, 0.0f,  1.0f,     0.75f,   0.01f };
    inline constexpr Spec detune       { "detune",    "Detune",    Kind::real, 0.0f,  40.0f,    12.0f,   0.01f };
    inline constexpr Spec sub          { "sub",       "Sub",       Kind::real, 0.0f,  1.0f,     0.35f,   0.001f };
    inline constexpr Spec cutoff       { "cutoff",    "Cutoff",    Kind::real, 40.0f, 18000.0f, 1800.0f, 0.01f, 0.35f };
    inline constexpr Spec resonance    { "resonance", "Resonance", Kind::real, 0.1f,  1.2f,     0.2f,    0.001f };
    inline constexpr Spec drive        { "drive",     "Drive",     Kind::real, 0.0f,  1.0f,     0.15f,   0.001f };
    inline constexpr Spec lfoRate      { "lfoRate",   "LFO Rate",  Kind::real, 0.05f, 20.0f,    0.8f,    0.001f, 0.35f };
    inline constexpr Spec lfoDepth     { "lfoDepth",  "LFO Depth", Kind::real, 0.0f,  1.0f,     0.25f,   0.001f };
    inline constexpr Spec lfoSync      = choice ("lfoSync", "LFO Sync", offOn, 0);
    inline constexpr Spec filter24db   = choice ("filter24db", "Filter 24dB", offOn, 0);
    inline constexpr Spec lfoSyncRate  = choice ("lfoSyncRate", "LFO Sync Rate", syncRates, 6);
    inline constexpr Spec glideTime    { "glideTime", "Glide Time", Kind::real, 0.0f, 2.0f, 0.0f, 0.001f, 0.5f };
    inline constexpr Spec unisonVoices = integer ("unisonVoices", "Unison Voices", 1, 8, 1);
    inline constexpr Spec oscAWave     = integer ("oscAWave", "Osc A Wave", 0, 2, 1);
    inline constexpr Spec oscBWave     = integer ("oscBWave", "Osc B Wave", 0, 2, 1);
    inline constexpr Spec attack       { "attack",  "Attack",  Kind::real, 0.001f, 0.1f, 0.01f, 0.001f, 0.5f };
    inline constexpr Spec decay        { "decay",   "Decay",   Kind::real, 0.01f,  1.0f, 0.08f, 0.001f, 0.4f };
    inline constexpr Spec sustain      { "sustain", "Sustain", Kind::real, 0.0f,   1.0f, 0.85f, 0.01f };
    inline constexpr Spec release      { "release", "Release", Kind::real, 0.01f,  2.0f, 0.2f,  0.001f, 0.4f };

    // The host's parameter order
    inline constexpr std::array all {
        &output, &detune, &sub, &cutoff, &resonance, &drive, &lfoRate, &lfoDepth,
        &lfoSync, &filter24db, &lfoSyncRate, &glideTime, &unisonVoices, &oscAWave, &oscBWave,
        &attack, &decay, &sustain, &release
    };

    static_assert (std::ranges::all_of (all, [] (const Spec* spec)
    {
        if (spec->kind == Kind::real)
            return true;

        return spec->step == 1.0f
            && (spec->kind == Kind::integer
                || (spec->min == 0.0f && static_cast<size_t> (spec->max) == spec->choices.size() - 1));
    }), "Integer and choice specs need step 1, and a choice spec's range is 0 to its last choice");
}
