#include <catch2/catch_test_macros.hpp>

#include "dsp/EffectCore.h"
#include "dsp/SignalsmithEngine.h"
#include "support/AllocationCounter.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace
{

// Stops the optimiser from deleting an allocation whose result is never used.
template <class T>
void keep (T& value)
{
    asm volatile ("" : : "g" (&value) : "memory");
}

std::vector<float> makeNoise (std::size_t length, std::uint32_t seed)
{
    std::vector<float> noise (length);
    std::uint32_t state = seed;

    for (auto& x : noise)
    {
        state = state * 1664525u + 1013904223u;
        x = (static_cast<float> (state >> 8) / 8388608.0f - 1.0f) * 0.5f;
    }

    return noise;
}

} // namespace

TEST_CASE ("AllocationScope really detects heap allocations", "[realtime][fixture]")
{
    std::size_t counted = 0;

    {
        testsupport::AllocationScope scope;

        std::vector<int> v (1000);
        keep (v);

        counted = scope.count();
    }

    REQUIRE (counted >= 1);
}

TEST_CASE ("AllocationScope counts nothing when nothing allocates", "[realtime][fixture]")
{
    std::vector<float> buffer (1000, 1.0f);
    std::size_t counted = 0;

    {
        testsupport::AllocationScope scope;

        for (auto& x : buffer)
            x *= 0.5f;

        keep (buffer);
        counted = scope.count();
    }

    REQUIRE (counted == 0);
}

TEST_CASE ("EffectCore with the Signalsmith engine does not allocate while processing", "[realtime]")
{
    constexpr int maxBlock = 512;

    fsdsp::EffectCore core (std::make_unique<fsdsp::SignalsmithEngine>());
    core.prepare (44100.0, maxBlock, 2);

    auto left = makeNoise (4096, 1u);
    auto right = makeNoise (4096, 2u);
    float* channels[] = { left.data(), right.data() };

    // Includes sizes above maxBlock, to exercise the chunking path.
    const int blockSizes[] = { 512, 64, 1, 37, 513, 1024, 128 };

    std::size_t allocations = 0;
    std::size_t firstSize = 0;

    {
        testsupport::AllocationScope scope;

        for (int round = 0; round < 40; ++round)
        {
            // Parameters change on the audio thread, as they do in the plugin.
            core.setPitchSemitones (static_cast<float> (round % 13) - 6.0f);
            core.setFormantSemitones (static_cast<float> (round % 7) - 3.0f);
            core.setMix (round % 2 == 0 ? 1.0f : 0.4f);
            core.setOutputGainDb (round % 3 == 0 ? 0.0f : -6.0f);

            for (const int frames : blockSizes)
                core.process (channels, 2, frames);
        }

        allocations = scope.count();
        firstSize = scope.firstSize();
    }

    INFO ("allocations: " << allocations << ", first one was " << firstSize << " bytes");
    REQUIRE (allocations == 0);
}

TEST_CASE ("EffectCore does not allocate when the input goes silent", "[realtime]")
{
    constexpr int maxBlock = 512;

    fsdsp::EffectCore core (std::make_unique<fsdsp::SignalsmithEngine>());
    core.prepare (44100.0, maxBlock, 2);

    auto left = makeNoise (maxBlock, 3u);
    auto right = makeNoise (maxBlock, 4u);
    float* channels[] = { left.data(), right.data() };

    for (int i = 0; i < 20; ++i)                       // warm the engine up with signal first
        core.process (channels, 2, maxBlock);

    std::fill (left.begin(), left.end(), 0.0f);
    std::fill (right.begin(), right.end(), 0.0f);

    std::size_t allocations = 0;

    {
        testsupport::AllocationScope scope;

        for (int i = 0; i < 400; ++i)                  // long enough to pass any silence-detection threshold
            core.process (channels, 2, maxBlock);

        allocations = scope.count();
    }

    INFO ("allocations: " << allocations);
    REQUIRE (allocations == 0);
}