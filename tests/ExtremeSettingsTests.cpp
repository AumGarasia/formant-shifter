#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "dsp/EffectCore.h"
#include "dsp/SignalsmithEngine.h"
#include "support/TestSignals.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

TEST_CASE ("Extreme pitch and formant settings give finite, bounded output", "[extremes]")
{
    const float pitch = GENERATE (-24.0f, 0.0f, 24.0f);
    const float formant = GENERATE (-12.0f, 0.0f, 12.0f);
    const bool link = GENERATE (false, true);

    constexpr int block = 512;

    fsdsp::EffectCore core (std::make_unique<fsdsp::SignalsmithEngine>());
    core.prepare (44100.0, block, 1);
    core.setFormantFollowsPitch (link);
    core.setPitchSemitones (pitch);
    core.setFormantSemitones (formant);
    core.setMix (1.0f);
    core.reset();

    auto audio = testsignals::makeNoise (44100, 5u);

    for (std::size_t start = 0; start < audio.size(); start += block)
    {
        const auto frames = static_cast<int> (std::min<std::size_t> (block, audio.size() - start));
        float* channels[] = { audio.data() + start };
        core.process (channels, 1, frames);
    }

    bool allFinite = true;
    float peak = 0.0f;

    for (const float x : audio)
    {
        allFinite = allFinite && std::isfinite (x);
        peak = std::max (peak, std::abs (x));
    }

    INFO ("pitch " << pitch << ", formant " << formant << ", link " << link << ", peak " << peak);
    REQUIRE (allFinite);
    REQUIRE (peak < 8.0f);
}