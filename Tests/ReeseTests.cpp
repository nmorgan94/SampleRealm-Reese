#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;

namespace
{
    // Sends a note on or off at the start of a block, the way a host would
    void processWithMidi (AudioPluginAudioProcessor& processor, juce::AudioBuffer<float>& buffer,
                          const juce::MidiMessage& message)
    {
        juce::MidiBuffer midi { message };
        processor.processBlock (buffer, midi);
    }
}

//==============================================================================
// What hosts and saved sessions see. Change it only on purpose
TEST_CASE_METHOD (ProcessorFixture, "Parameters match the snapshot hosts rely on", "[parameters]")
{
    struct Expected
    {
        const char* id;
        int versionHint;
        const char* name;
        const char* label;
        float start, end, interval, skew, defaultValue;
        std::array<const char*, 3> textAt0HalfAnd1;
    };

    const std::vector<Expected> snapshot {
        { "output",       1, "Output",        "", 0.0f,   1.0f,     0.01f,  1.0f,  0.75f,   { "0.00", "0.50", "1.00" } },
        { "detune",       1, "Detune",        "", 0.0f,   40.0f,    0.01f,  1.0f,  12.0f,   { "0.00", "20.00", "40.00" } },
        { "sub",          1, "Sub",           "", 0.0f,   1.0f,     0.001f, 1.0f,  0.35f,   { "0.000", "0.500", "1.000" } },
        { "cutoff",       1, "Cutoff",        "", 40.0f,  18000.0f, 0.01f,  0.35f, 1800.0f, { "40.00", "2518.68", "18000.00" } },
        { "resonance",    1, "Resonance",     "", 0.1f,   1.2f,     0.001f, 1.0f,  0.2f,    { "0.100", "0.650", "1.200" } },
        { "drive",        1, "Drive",         "", 0.0f,   1.0f,     0.001f, 1.0f,  0.15f,   { "0.000", "0.500", "1.000" } },
        { "lfoRate",      1, "LFO Rate",      "", 0.05f,  20.0f,    0.001f, 0.35f, 0.8f,    { "0.050", "2.803", "20.000" } },
        { "lfoDepth",     1, "LFO Depth",     "", 0.0f,   1.0f,     0.001f, 1.0f,  0.25f,   { "0.000", "0.500", "1.000" } },
        { "lfoSync",      1, "LFO Sync",      "", 0.0f,   1.0f,     1.0f,   1.0f,  0.0f,    { "Off", "Off", "On" } },
        { "filter24db",   1, "Filter 24dB",   "", 0.0f,   1.0f,     1.0f,   1.0f,  0.0f,    { "Off", "Off", "On" } },
        { "lfoSyncRate",  1, "LFO Sync Rate", "", 0.0f,   14.0f,    1.0f,   1.0f,  6.0f,    { "1/16", "1/4T", "4 Bars" } },
        { "glideTime",    1, "Glide Time",    "", 0.0f,   2.0f,     0.001f, 0.5f,  0.0f,    { "0.000", "0.500", "2.000" } },
        { "unisonVoices", 1, "Unison Voices", "", 1.0f,   8.0f,     1.0f,   1.0f,  1.0f,    { "1", "4", "8" } },
        { "oscAWave",     1, "Osc A Wave",    "", 0.0f,   2.0f,     1.0f,   1.0f,  1.0f,    { "0", "1", "2" } },
        { "oscBWave",     1, "Osc B Wave",    "", 0.0f,   2.0f,     1.0f,   1.0f,  1.0f,    { "0", "1", "2" } },
        { "attack",       1, "Attack",        "", 0.001f, 0.1f,     0.001f, 0.5f,  0.01f,   { "0.001", "0.026", "0.100" } },
        { "decay",        1, "Decay",         "", 0.01f,  1.0f,     0.001f, 0.4f,  0.08f,   { "0.010", "0.185", "1.000" } },
        { "sustain",      1, "Sustain",       "", 0.0f,   1.0f,     0.01f,  1.0f,  0.85f,   { "0.00", "0.50", "1.00" } },
        { "release",      1, "Release",       "", 0.01f,  2.0f,     0.001f, 0.4f,  0.2f,    { "0.010", "0.362", "2.000" } },
    };

    const auto& parameters = processor.getParameters();
    REQUIRE (static_cast<size_t> (parameters.size()) == snapshot.size());

    for (size_t i = 0; i < snapshot.size(); ++i)
    {
        const auto& expected = snapshot[i];
        CAPTURE (i, expected.id);

        auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (parameters[static_cast<int> (i)]);
        REQUIRE (parameter != nullptr);

        CHECK (parameter->getParameterID() == expected.id);
        CHECK (parameter->getVersionHint() == expected.versionHint);
        CHECK (parameter->getName (100) == expected.name);
        CHECK (parameter->getLabel() == expected.label);

        const auto& range = parameter->getNormalisableRange();
        CHECK_THAT (range.start, WithinAbs (expected.start, 1.0e-6));
        CHECK_THAT (range.end, WithinAbs (expected.end, 1.0e-6));
        CHECK_THAT (range.interval, WithinAbs (expected.interval, 1.0e-6));
        CHECK_THAT (range.skew, WithinAbs (expected.skew, 1.0e-6));
        // Skewed ranges round the default slightly (cutoff reads back as 1799.9999)
        CHECK_THAT (parameter->convertFrom0to1 (parameter->getDefaultValue()),
                    WithinAbs (expected.defaultValue, (expected.end - expected.start) * 1.0e-5));

        CHECK (parameter->getText (0.0f, 100) == expected.textAt0HalfAnd1[0]);
        CHECK (parameter->getText (0.5f, 100) == expected.textAt0HalfAnd1[1]);
        CHECK (parameter->getText (1.0f, 100) == expected.textAt0HalfAnd1[2]);
    }
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "Only mono or stereo output is supported", "[buses]")
{
    using Set = juce::AudioChannelSet;

    // A synth has no input bus, so each layout is { {}, { output bus } }
    CHECK (processor.isBusesLayoutSupported ({ {}, { Set::mono() } }));
    CHECK (processor.isBusesLayoutSupported ({ {}, { Set::stereo() } }));

    CHECK_FALSE (processor.isBusesLayoutSupported ({ {}, { Set::create5point1() } }));
    CHECK_FALSE (processor.isBusesLayoutSupported ({ {}, { Set::disabled() } }));
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "Output stays finite across block sizes with all parameters at min, mid or max", "[dsp]")
{
    // Includes a block larger than the size given to prepareToPlay, which some hosts send
    const auto numSamples = GENERATE (1, 64, blockSize, blockSize * 4);

    // Every parameter at its minimum, middle and maximum, so switches and choices flip too
    const auto normalisedValue = GENERATE (0.0f, 0.5f, 1.0f);

    CAPTURE (numSamples, normalisedValue);

    for (auto* parameter : processor.getParameters())
        parameter->setValueNotifyingHost (normalisedValue);

    juce::AudioBuffer<float> buffer (2, numSamples);

    // A note, a second overlapping note (legato and glide), both released, then the tail
    const juce::MidiMessage messages[] = {
        juce::MidiMessage::noteOn (1, 48, 1.0f),
        juce::MidiMessage::noteOn (1, 55, 1.0f),
        juce::MidiMessage::noteOff (1, 48),
        juce::MidiMessage::noteOff (1, 55),
    };

    for (const auto& message : messages)
    {
        processWithMidi (processor, buffer, message);
        CHECK (allSamplesFinite (buffer));
    }

    for (int block = 0; block < 4; ++block)
    {
        process (processor, buffer);
        CHECK (allSamplesFinite (buffer));
    }
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "Opening the editor drops peaks played while it was closed", "[editor]")
{
    juce::AudioBuffer<float> loud { 2, blockSize };
    loud.clear();
    loud.setSample (0, 0, 2.0f);
    processor.getOutputMeter().process (loud);

    const std::unique_ptr<juce::AudioProcessorEditor> editor { processor.createEditor() };

    CHECK (processor.getOutputMeter().readAndReset (0) == 0.0f);
}

TEST_CASE_METHOD (ProcessorFixture, "State round trips into a new instance", "[state]")
{
    // 0.9 is off the default for every parameter, and turns both switches on
    for (auto* parameter : processor.getParameters())
        parameter->setValueNotifyingHost (0.9f);

    juce::MemoryBlock state;
    processor.getStateInformation (state);

    AudioPluginAudioProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

    const auto& original = processor.getParameters();
    const auto& copy = restored.getParameters();
    REQUIRE (copy.size() == original.size());

    for (int i = 0; i < original.size(); ++i)
    {
        CAPTURE (original[i]->getName (100));
        CHECK_THAT (copy[i]->getValue(), WithinAbs (original[i]->getValue(), 1.0e-5));
    }
}

TEST_CASE_METHOD (ProcessorFixture, "Invalid state is ignored", "[state]")
{
    setParameter (processor, Parameters::cutoffId, 500.0f);
    const auto before = getParameter (processor, Parameters::cutoffId);

    SECTION ("Random bytes")
    {
        const char garbage[] = "definitely not plugin state";
        processor.setStateInformation (garbage, static_cast<int> (sizeof (garbage)));
    }

    SECTION ("Empty data")
    {
        processor.setStateInformation (nullptr, 0);
    }

    SECTION ("Valid XML with the wrong root tag")
    {
        juce::MemoryBlock state;
        juce::AudioProcessor::copyXmlToBinary (juce::XmlElement ("SomethingElse"), state);
        processor.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    }

    CHECK_THAT (getParameter (processor, Parameters::cutoffId), WithinAbs (before, 1.0e-6));
}
