#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "dsp/EngineFactory.h"
#include "dsp/OfflineRenderer.h"
#include "support/VowelProbe.h"

#include <cmath>
#include <string>
#include <vector>

namespace
{
constexpr int kBlock = 512;

struct Measurement
{
    double f0Ratio;       // output pitch / input pitch
    double formantRatio;  // output spectral centroid / input spectral centroid
};

// Renders the synthetic vowel through an engine and measures the result against the
// unprocessed input. `pitchFirst` selects the order the two settings are applied in.
Measurement measure (const std::string& engineName, float pitch, float formant, bool pitchFirst)
{
    static const auto input = vowelprobe::makeVowel();
    static const double f0In = vowelprobe::estimateF0 (input);
    static const double centroidIn = vowelprobe::spectralCentroid (input);

    auto engine = fsdsp::makeEngine (engineName);
    engine->prepare (vowelprobe::kSampleRate, kBlock, 1);

    if (pitchFirst)
    {
        engine->setPitchSemitones (pitch);
        engine->setFormantSemitones (formant);
    }
    else
    {
        engine->setFormantSemitones (formant);
        engine->setPitchSemitones (pitch);
    }

    const fsdsp::PlanarAudio audio (1, input);
    const auto output = fsdsp::renderAligned (*engine, audio, kBlock)[0];

    return { vowelprobe::estimateF0 (output) / f0In,
             vowelprobe::spectralCentroid (output) / centroidIn };
}
} // namespace

TEST_CASE ("The default engine moves pitch without moving formants", "[formant]")
{
    const bool pitchFirst = GENERATE (true, false);
    const float pitch = GENERATE (7.0f, -5.0f);

    const auto m = measure ("signalsmith", pitch, 0.0f, pitchFirst);

    REQUIRE (m.f0Ratio == Catch::Approx (std::pow (2.0, static_cast<double> (pitch) / 12.0)).epsilon (0.02));
    REQUIRE (m.formantRatio == Catch::Approx (1.0).epsilon (0.05));
}

TEST_CASE ("The default engine moves formants without moving pitch", "[formant]")
{
    const float formant = GENERATE (5.0f, -5.0f);

    const auto m = measure ("signalsmith", 0.0f, formant, true);

    REQUIRE (m.f0Ratio == Catch::Approx (1.0).epsilon (0.02));
    REQUIRE (m.formantRatio == Catch::Approx (std::pow (2.0, static_cast<double> (formant) / 12.0)).epsilon (0.06));
}

TEST_CASE ("The default engine applies pitch and formant independently when both are set", "[formant]")
{
    // The harder order: formant first, then pitch (the order the plugin hits when the pitch knob moves).
    const auto m = measure ("signalsmith", 7.0f, 5.0f, false);

    REQUIRE (m.f0Ratio == Catch::Approx (std::pow (2.0, 7.0 / 12.0)).epsilon (0.02));
    REQUIRE (m.formantRatio == Catch::Approx (std::pow (2.0, 5.0 / 12.0)).epsilon (0.06));
}

TEST_CASE ("The coupled variant lets formants follow the pitch", "[formant]")
{
    // Documents the library's other mode, and fails loudly if a library update changes its meaning.
    const auto m = measure ("signalsmith-coupled", 7.0f, 0.0f, true);

    REQUIRE (m.formantRatio > 1.25);
}