#include <catch2/catch_test_macros.hpp>

#include "dsp/PassthroughEngine.h"

#include <cstddef>
#include <vector>

namespace
{
    constexpr int kBlock = 512;

    std::vector<float> makeRamp (float scale)
    {
        std::vector<float> v (static_cast<std::size_t> (kBlock));
        for (std::size_t i = 0; i < v.size(); ++i)
            v[i] = static_cast<float> (i) * scale;
        return v;
    }   // namespace
}

TEST_CASE ("PassthroughEngine copies input to output unchanged", "[PassthroughEngine]")
{
    fsdsp::PassthroughEngine engine;
    engine.prepare (48000.0, kBlock, 2);

    const auto inputL = makeRamp (0.001f);
    const auto inputR = makeRamp (-0.001f);
    std::vector<float> outputL (inputL.size(), 0.0f);
    std::vector<float> outputR (inputR.size(), 0.0f);

    const float* inputs[] = { inputL.data(), inputR.data() };
    float* outputs[] = { outputL.data(), outputR.data() };

    engine.process (inputs, outputs, 2, kBlock);

    REQUIRE (outputL == inputL);
    REQUIRE (outputR == inputR);
}

TEST_CASE ("PassthroughEngine supports in-place processing", "[PassthroughEngine]")
{
    fsdsp::PassthroughEngine engine;
    engine.prepare (48000.0, kBlock, 1);

    const auto expected = makeRamp (0.002f);
    auto buffer = expected;

    float* channels[] = { buffer.data() };
    engine.process (channels, channels, 1, kBlock);

    REQUIRE (buffer == expected);
}

TEST_CASE ("PassthroughEngine reports zero latency", "[PassthroughEngine]")
{
    fsdsp::PassthroughEngine engine;
    engine.prepare (48000.0, kBlock, 2);

    REQUIRE (engine.getLatencySamples() == 0);
}