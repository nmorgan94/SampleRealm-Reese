#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;

namespace
{
    // Reese's parameters use raw string IDs; the version mirrors versionHint in Parameters.cpp
    juce::ParameterID id (const char* paramID) { return { paramID, 1 }; }

    // Sends a note on or off at the start of a block, the way a host would
    void processWithMidi (AudioPluginAudioProcessor& processor, juce::AudioBuffer<float>& buffer,
                          const juce::MidiMessage& message)
    {
        juce::MidiBuffer midi { message };
        processor.processBlock (buffer, midi);
    }

    struct Expected { const char* id; float start, end, defaultValue; };

    const auto expectedParameters = std::to_array<Expected> ({
        { "output",       0.0f,   1.0f,     0.75f },
        { "detune",       0.0f,   40.0f,    12.0f },
        { "sub",          0.0f,   1.0f,     0.35f },
        { "cutoff",       40.0f,  18000.0f, 1800.0f },
        { "resonance",    0.1f,   1.2f,     0.2f },
        { "drive",        0.0f,   1.0f,     0.15f },
        { "lfoRate",      0.05f,  20.0f,    0.8f },
        { "lfoDepth",     0.0f,   1.0f,     0.25f },
        { "lfoSync",      0.0f,   1.0f,     0.0f },
        { "filter24db",   0.0f,   1.0f,     0.0f },
        { "lfoSyncRate",  0.0f,   14.0f,    6.0f },
        { "glideTime",    0.0f,   2.0f,     0.0f },
        { "unisonVoices", 1.0f,   8.0f,     1.0f },
        { "oscAWave",     0.0f,   2.0f,     1.0f },
        { "oscBWave",     0.0f,   2.0f,     1.0f },
        { "attack",       0.001f, 0.1f,     0.01f },
        { "decay",        0.01f,  1.0f,     0.08f },
        { "sustain",      0.0f,   1.0f,     0.85f },
        { "release",      0.01f,  2.0f,     0.2f },
    });
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "Parameters have the expected ranges and defaults", "[parameters]")
{
    const auto expected = GENERATE (from_range (expectedParameters));

    CAPTURE (expected.id);

    auto* parameter = processor.getAPVTS().getParameter (expected.id);
    REQUIRE (parameter != nullptr);

    const auto& range = parameter->getNormalisableRange();
    CHECK_THAT (range.start, WithinAbs (expected.start, 1.0e-6));
    CHECK_THAT (range.end, WithinAbs (expected.end, 1.0e-6));
    // Skewed ranges round the default slightly (cutoff reads back as 1799.9999), so scale the tolerance
    const auto tolerance = (expected.end - expected.start) * 1.0e-5;
    CHECK_THAT (getParameter (processor, id (expected.id)), WithinAbs (expected.defaultValue, tolerance));
}

TEST_CASE_METHOD (ProcessorFixture, "Every parameter is covered by the range test", "[parameters]")
{
    // Fails when a parameter is added without a row in expectedParameters
    for (auto* parameter : processor.getParameters())
    {
        auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (parameter);
        REQUIRE (withId != nullptr);
        CAPTURE (withId->paramID);

        CHECK (std::ranges::any_of (expectedParameters, [&] (const Expected& e) { return withId->paramID == e.id; }));
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
    setParameter (processor, id ("cutoff"), 500.0f);
    const auto before = getParameter (processor, id ("cutoff"));

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

    CHECK_THAT (getParameter (processor, id ("cutoff")), WithinAbs (before, 1.0e-6));
}
