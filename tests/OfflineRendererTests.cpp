#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "dsp/OfflineRenderer.h"
#include "dsp/PassthroughEngine.h"
#include "support/DelayEngine.h"

#include <cstddef>
#include <vector>

namespace
{
// Non-zero, distinct per sample, different per channel. A bug that outputs
// silence or shifts the signal by even one sample cannot match this.
fsdsp::PlanarAudio makeInput (std::size_t numChannels, std::size_t numSamples)
{
    fsdsp::PlanarAudio audio (numChannels, std::vector<float> (numSamples));

    for (std::size_t ch = 0; ch < numChannels; ++ch)
        for (std::size_t i = 0; i < numSamples; ++i)
            audio[ch][i] = static_cast<float> (i + 1) * (ch % 2 == 0 ? 1.0f : -1.0f);

    return audio;
}
} // namespace

TEST_CASE ("DelayEngine delays by exactly its latency", "[renderer][fixture]")
{
    constexpr int delay = 5;
    constexpr int frames = 20;

    testsupport::DelayEngine engine (delay);
    engine.prepare (48000.0, frames, 1);

    const auto in = makeInput (1, frames);
    std::vector<float> out (frames, -1.0f);

    const float* inputs[] = { in[0].data() };
    float* outputs[] = { out.data() };
    engine.process (inputs, outputs, 1, frames);

    for (int i = 0; i < frames; ++i)
    {
        const auto idx = static_cast<std::size_t> (i);
        const float expected = i < delay ? 0.0f : in[0][static_cast<std::size_t> (i - delay)];
        REQUIRE (out[idx] == expected);
    }
}

TEST_CASE ("renderAligned returns the input for a zero-latency engine", "[renderer]")
{
    fsdsp::PassthroughEngine engine;
    engine.prepare (44100.0, 512, 2);

    const auto input = makeInput (2, 1237);

    REQUIRE (fsdsp::renderAligned (engine, input, 512) == input);
}

TEST_CASE ("renderAligned removes engine latency", "[renderer]")
{
    const int delay = GENERATE (1, 7, 64, 511, 512, 513, 1000);
    const std::size_t length = GENERATE (std::size_t { 1 }, std::size_t { 10 }, std::size_t { 1237 });

    testsupport::DelayEngine engine (delay);
    engine.prepare (44100.0, 512, 2);

    const auto input = makeInput (2, length);
    const auto output = fsdsp::renderAligned (engine, input, 512);

    REQUIRE (output.size() == input.size());
    REQUIRE (output[0].size() == length);
    REQUIRE (output == input);
}

TEST_CASE ("renderAligned result does not depend on block size", "[renderer]")
{
    const int blockSize = GENERATE (1, 33, 512, 4096);

    testsupport::DelayEngine engine (100);
    engine.prepare (44100.0, 4096, 2);

    const auto input = makeInput (2, 1237);

    REQUIRE (fsdsp::renderAligned (engine, input, blockSize) == input);
}

TEST_CASE ("renderAligned gives the same result when reusing an engine", "[renderer]")
{
    testsupport::DelayEngine engine (64);
    engine.prepare (44100.0, 512, 1);

    const auto input = makeInput (1, 700);

    const auto first = fsdsp::renderAligned (engine, input, 512);
    const auto second = fsdsp::renderAligned (engine, input, 512);

    REQUIRE (first == second);
    REQUIRE (second == input);
}