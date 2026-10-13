#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "dsp/ProcessChunked.h"
#include "support/DelayEngine.h"
#include "support/RecordingEngine.h"

#include <cstddef>
#include <vector>

namespace
{
using Planar = std::vector<std::vector<float>>;

// Non-zero, distinct per sample, different per channel.
Planar makeInput (std::size_t numChannels, std::size_t numSamples)
{
    Planar audio (numChannels, std::vector<float> (numSamples));

    for (std::size_t ch = 0; ch < numChannels; ++ch)
        for (std::size_t i = 0; i < numSamples; ++i)
            audio[ch][i] = static_cast<float> (i + 1) * (ch % 2 == 0 ? 1.0f : -1.0f);

    return audio;
}
} // namespace

TEST_CASE ("processInPlaceChunked matches single-call processing", "[chunked]")
{
    const int maxChunk = GENERATE (1, 33, 64, 512, 4096);
    constexpr std::size_t length = 1237;
    const auto frames = static_cast<int> (length);

    const auto input = makeInput (2, length);

    // Reference: one big call, out of place.
    testsupport::DelayEngine reference (100);
    reference.prepare (44100.0, 4096, 2);
    auto expected = input;
    {
        const float* in[] = { input[0].data(), input[1].data() };
        float* out[] = { expected[0].data(), expected[1].data() };
        reference.process (in, out, 2, frames);
    }

    // Under test: in place, in chunks.
    testsupport::DelayEngine engine (100);
    engine.prepare (44100.0, maxChunk, 2);
    auto actual = input;
    float* channels[] = { actual[0].data(), actual[1].data() };
    fsdsp::processInPlaceChunked (engine, channels, 2, frames, maxChunk);

    REQUIRE (expected != input);   // the delay really changed the signal
    REQUIRE (actual == expected);
}

TEST_CASE ("processInPlaceChunked never exceeds maxChunk frames per call", "[chunked]")
{
    testsupport::RecordingEngine engine;
    std::vector<float> left (1000), right (1000);
    float* channels[] = { left.data(), right.data() };

    fsdsp::processInPlaceChunked (engine, channels, 2, 1000, 128);

    REQUIRE (engine.largest == 128);
    REQUIRE (engine.totalFrames == 1000);
    REQUIRE (engine.calls == 8);   // ceil (1000 / 128)
}

TEST_CASE ("processInPlaceChunked does nothing for zero frames", "[chunked]")
{
    testsupport::RecordingEngine engine;
    std::vector<float> left (16), right (16);
    float* channels[] = { left.data(), right.data() };

    fsdsp::processInPlaceChunked (engine, channels, 2, 0, 128);

    REQUIRE (engine.calls == 0);
}