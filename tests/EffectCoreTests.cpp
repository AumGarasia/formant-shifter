#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "dsp/EffectCore.h"
#include "support/DelayEngine.h"
#include "support/RecordingEngine.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace
{
using Planar = std::vector<std::vector<float>>;

constexpr int kLatency = 100;

// Non-zero, distinct per sample, different per channel.
Planar makeInput (std::size_t numChannels, std::size_t numSamples)
{
    Planar audio (numChannels, std::vector<float> (numSamples));

    for (std::size_t ch = 0; ch < numChannels; ++ch)
        for (std::size_t i = 0; i < numSamples; ++i)
            audio[ch][i] = static_cast<float> (i + 1) * (ch % 2 == 0 ? 1.0f : -1.0f);

    return audio;
}

// What a pure delay of `delay` samples, scaled by `gain`, does to `input`.
Planar delayed (const Planar& input, int delay, float gain)
{
    const auto d = static_cast<std::size_t> (delay);
    Planar out (input.size(), std::vector<float> (input[0].size(), 0.0f));

    for (std::size_t ch = 0; ch < input.size(); ++ch)
        for (std::size_t i = d; i < input[ch].size(); ++i)
            out[ch][i] = gain * input[ch][i - d];

    return out;
}

// Runs `audio` through `core` in a single call, in place.
Planar run (fsdsp::EffectCore& core, Planar audio)
{
    std::vector<float*> channels;

    for (auto& ch : audio)
        channels.push_back (ch.data());

    core.process (channels.data(), static_cast<int> (audio.size()), static_cast<int> (audio[0].size()));
    return audio;
}

// The engine delays by kLatency and inverts: wet = -dry, so the two paths are
// distinguishable, and they cancel exactly only if they are aligned to the sample.
fsdsp::EffectCore makeCore (int maxBlock, int channels)
{
    fsdsp::EffectCore core (std::make_unique<testsupport::DelayEngine> (kLatency, -1.0f));
    core.prepare (44100.0, maxBlock, channels);
    return core;
}
} // namespace

TEST_CASE ("EffectCore reports the engine's latency", "[core]")
{
    auto core = makeCore (512, 2);

    REQUIRE (core.getLatencySamples() == kLatency);
}

TEST_CASE ("EffectCore mix 1 outputs only the wet signal, exactly", "[core]")
{
    auto core = makeCore (512, 2);
    core.setMix (1.0f);
    core.reset();

    const auto input = makeInput (2, 1237);

    REQUIRE (run (core, input) == delayed (input, kLatency, -1.0f));
}

TEST_CASE ("EffectCore mix 0 outputs only the dry signal, exactly, delayed to match", "[core]")
{
    auto core = makeCore (512, 2);
    core.setMix (0.0f);
    core.reset();

    const auto input = makeInput (2, 1237);

    REQUIRE (run (core, input) == delayed (input, kLatency, 1.0f));
}

TEST_CASE ("EffectCore dry and wet paths are aligned to the sample", "[core]")
{
    auto core = makeCore (512, 2);
    core.setMix (0.5f);
    core.reset();

    const auto input = makeInput (2, 1237);
    const Planar silence (2, std::vector<float> (1237, 0.0f));

    REQUIRE (run (core, input) == silence);
}

TEST_CASE ("EffectCore applies output gain in decibels", "[core]")
{
    auto core = makeCore (512, 1);
    core.setMix (0.0f);                    // dry only: output = input delayed
    core.setOutputGainDb (-20.0f);
    core.reset();

    const auto input = makeInput (1, 1000);
    const auto expected = delayed (input, kLatency, 0.1f);
    const auto actual = run (core, input);

    for (std::size_t i = 0; i < actual[0].size(); ++i)
        REQUIRE (actual[0][i] == Catch::Approx (expected[0][i]).epsilon (1e-5).margin (1e-6));
}

TEST_CASE ("EffectCore ramps mix changes instead of jumping", "[core]")
{
    // Zero-latency inverting engine and a constant input of 1: output = 1 - 2 * mix.
    fsdsp::EffectCore core (std::make_unique<testsupport::DelayEngine> (0, -1.0f));
    core.prepare (44100.0, 512, 1);
    core.setMix (0.0f);
    core.reset();
    core.setMix (1.0f);                    // the ramp starts here

    std::vector<float> audio (4096, 1.0f);
    float* channels[] = { audio.data() };
    core.process (channels, 1, 4096);

    constexpr float rampSamples = 882.0f;  // 20 ms at 44.1 kHz
    const float maxStep = 2.0f / rampSamples * 1.01f;

    float worstStep = 0.0f;
    for (std::size_t i = 1; i < audio.size(); ++i)
        worstStep = std::max (worstStep, std::abs (audio[i] - audio[i - 1]));

    REQUIRE (worstStep <= maxStep);
    REQUIRE (audio.front() < 1.0f);        // already moving on the first sample
    REQUIRE (audio.back() == Catch::Approx (-1.0f));
}

TEST_CASE ("EffectCore handles blocks larger than the prepared maximum", "[core]")
{
    const int maxBlock = GENERATE (1, 33, 64, 512);
    const auto input = makeInput (2, 1237);

    auto reference = makeCore (4096, 2);   // the whole signal fits in one chunk
    auto chunked = makeCore (maxBlock, 2);

    for (auto* core : { &reference, &chunked })
    {
        core->setMix (0.3f);
        core->setOutputGainDb (-3.0f);
        core->reset();
    }

    REQUIRE (run (chunked, input) == run (reference, input));
}

TEST_CASE ("EffectCore reset clears the dry path", "[core]")
{
    auto core = makeCore (512, 2);
    core.setMix (0.0f);
    core.reset();

    const auto input = makeInput (2, 700);

    const auto first = run (core, input);
    core.reset();
    const auto second = run (core, input);

    REQUIRE (second == first);
}

TEST_CASE ("EffectCore forwards pitch and formant to the engine", "[core]")
{
    auto engine = std::make_unique<testsupport::RecordingEngine>();
    auto* recorder = engine.get();
    fsdsp::EffectCore core (std::move (engine));

    core.setPitchSemitones (3.0f);         // before prepare: prepare must apply it
    core.setFormantSemitones (-2.0f);
    core.prepare (44100.0, 512, 2);

    REQUIRE (recorder->lastPitch == Catch::Approx (3.0f));
    REQUIRE (recorder->lastFormant == Catch::Approx (-2.0f));

    core.setPitchSemitones (7.0f);         // after prepare: forwarded immediately

    REQUIRE (recorder->lastPitch == Catch::Approx (7.0f));
}

TEST_CASE ("EffectCore never hands the engine more than the prepared maximum", "[core]")
{
    auto engine = std::make_unique<testsupport::RecordingEngine>();
    auto* recorder = engine.get();
    fsdsp::EffectCore core (std::move (engine));
    core.prepare (44100.0, 128, 2);

    std::vector<float> left (1000), right (1000);
    float* channels[] = { left.data(), right.data() };
    core.process (channels, 2, 1000);

    REQUIRE (recorder->largest == 128);
    REQUIRE (recorder->totalFrames == 1000);
    REQUIRE (recorder->calls == 8);     // ceil (1000 / 128)
}

TEST_CASE ("EffectCore with formant link adds the pitch to the formant", "[core][link]")
{
    auto engine = std::make_unique<testsupport::RecordingEngine>();
    auto* recorder = engine.get();
    fsdsp::EffectCore core (std::move (engine));
    core.prepare (44100.0, 512, 2);

    core.setFormantFollowsPitch (true);
    core.setPitchSemitones (5.0f);
    core.setFormantSemitones (2.0f);

    REQUIRE (recorder->lastPitch == Catch::Approx (5.0f));
    REQUIRE (recorder->lastFormant == Catch::Approx (7.0f));       // 5 + 2

    core.setPitchSemitones (-3.0f);                                // the formant control is untouched
    REQUIRE (recorder->lastFormant == Catch::Approx (-1.0f));      // -3 + 2

    core.setFormantFollowsPitch (false);                           // independent again
    REQUIRE (recorder->lastPitch == Catch::Approx (-3.0f));
    REQUIRE (recorder->lastFormant == Catch::Approx (2.0f));
}

TEST_CASE ("EffectCore without formant link ignores the pitch for the formant", "[core][link]")
{
    auto engine = std::make_unique<testsupport::RecordingEngine>();
    auto* recorder = engine.get();
    fsdsp::EffectCore core (std::move (engine));
    core.prepare (44100.0, 512, 2);

    core.setFormantSemitones (2.0f);
    core.setPitchSemitones (10.0f);

    REQUIRE (recorder->lastFormant == Catch::Approx (2.0f));
}

TEST_CASE ("EffectCore applies the formant link when it was set before prepare", "[core][link]")
{
    auto engine = std::make_unique<testsupport::RecordingEngine>();
    auto* recorder = engine.get();
    fsdsp::EffectCore core (std::move (engine));

    core.setFormantFollowsPitch (true);
    core.setPitchSemitones (4.0f);
    core.setFormantSemitones (1.0f);
    core.prepare (44100.0, 512, 2);

    REQUIRE (recorder->lastFormant == Catch::Approx (5.0f));
}

TEST_CASE ("EffectCore clamps the linked formant shift", "[core][link]")
{
    auto engine = std::make_unique<testsupport::RecordingEngine>();
    auto* recorder = engine.get();
    fsdsp::EffectCore core (std::move (engine));
    core.prepare (44100.0, 512, 2);

    core.setFormantFollowsPitch (true);

    core.setPitchSemitones (24.0f);
    core.setFormantSemitones (12.0f);                              // 36 -> clamped
    REQUIRE (recorder->lastFormant == Catch::Approx (24.0f));

    core.setPitchSemitones (-24.0f);
    core.setFormantSemitones (-12.0f);                             // -36 -> clamped
    REQUIRE (recorder->lastFormant == Catch::Approx (-24.0f));
}