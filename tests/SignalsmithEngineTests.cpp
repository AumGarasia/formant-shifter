#include <catch2/catch_test_macros.hpp>

#include "dsp/OfflineRenderer.h"
#include "dsp/SignalsmithEngine.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace
{

constexpr std::size_t kBlock = 512;

// Deterministic noise in [-0.5, 0.5], different per seed.
fsdsp::PlanarAudio makeNoise (std::size_t numChannels, std::size_t numSamples, std::uint32_t seed)
{
    fsdsp::PlanarAudio audio (numChannels, std::vector<float> (numSamples));
    std::uint32_t state = seed;

    for (auto& channel : audio)
        for (auto& x : channel)
        {
            state = state * 1664525u + 1013904223u;
            x = (static_cast<float> (state >> 8) / 8388608.0f - 1.0f) * 0.5f;
        }
    
    return audio;
}

} // namespace

TEST_CASE("SignalsmithEngine reports its latency accurately", "[signalsmith]")
{
    constexpr std::size_t burstStart = 3000;
    constexpr std::size_t burstLength = 256;

    fsdsp::SignalsmithEngine engine;
    engine.prepare (44100.0, static_cast<int> (kBlock), 1);

    const auto latency = static_cast<std::size_t> (engine.getLatencySamples());
    REQUIRE (latency > 0);

    // A noise burst in silence is easy to locate precisely in the output.
    const auto burst = makeNoise (1, burstLength, 99u)[0];
    const std::size_t total = burstStart + burstLength + 2 * latency + 4096;

    std::vector<float> input (total, 0.0f);
    for (std::size_t i = 0; i < burstLength; ++i)
        input[burstStart + i] = burst[i];

    std::vector<float> output (total, 0.0f);

    for (std::size_t start = 0; start < total; start += kBlock)
    {
        const auto frames = static_cast<int> (std::min (kBlock, total - start));
        const float* in[] = { input.data() + start };
        float* out[] = { output.data() + start };
        engine.process (in, out, 1, frames);
    }

    // Find the delay that best lines the burst up with the output.
    double burstEnergy = 0.0;
    for (const float x : burst)
        burstEnergy += static_cast<double> (x) * static_cast<double> (x);
    
    std::size_t bestLag = 0;
    double bestScore = -1.0;

    for (std::size_t lag = 0; lag <= 2 * latency; ++lag)
    {
        double score = 0.0;
        for (std::size_t i = 0; i < burstLength; ++i)
            score += static_cast<double> (burst[i]) * static_cast<double> (output[burstStart + lag + i]);
        
        if (score > bestScore)
        {
            bestScore = score;
            bestLag = lag;
        }
    }

    INFO ("Reported latency: " << latency << ", measured: " << bestLag
          << ", match quality: " << bestScore / burstEnergy << " (1.0 = perfect copy)");
    
    const auto error = static_cast<std::ptrdiff_t> (bestLag) - static_cast<std::ptrdiff_t> (latency);
    REQUIRE (std::abs (error) <= 2);
}

TEST_CASE ("SignalsmithEngine give identical output in place and out of place", "[signalsmith]")
{
    constexpr std::size_t length = 20000;
    const auto input = makeNoise (2, length, 7u);

    auto setUp = [] (fsdsp::SignalsmithEngine& engine)
    {
        engine.prepare (44100.0, static_cast<int> (kBlock), 2);
        engine.setPitchSemitones(3.0f);
        engine.setFormantSemitones(-2.0f);
    };

    // Two fresh engines with the same fixed seed must behave identically.
    fsdsp::SignalsmithEngine separate, inPlace;
    setUp (separate);
    setUp (inPlace);

    auto outSeparate = input;   // overwritten by the engine, never aliases 'input' 
    auto outInPlace = input;    // processed directly in its own buffers

    for (std::size_t start = 0; start < length; start += kBlock)
    {
        const auto frames = static_cast<int> (std::min (kBlock, length - start));

        const float* in[] = { input[0].data() + start, input[1].data() + start };
        float* out[] = { outSeparate[0].data() + start, outSeparate[1].data() + start };
        float* both[] = { outInPlace[0].data() + start, outInPlace[1].data() + start };

        separate.process (in, out, 2, frames);
        inPlace.process (both, both, 2, frames);
    }

    REQUIRE (outSeparate != input);         // the engine really did something
    REQUIRE (outInPlace == outSeparate);    // and aliasing did not change the result. 
}