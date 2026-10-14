// Measures how an engine's pitch and formant controls behave on a synthetic vowel.
// Run it in a Release build.

#include "VowelProbe.h"

#include "dsp/EngineFactory.h"
#include "dsp/OfflineRenderer.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
constexpr int kBlock = 512;

std::vector<float> render (const std::string& engineName, float pitch, float formant,
                           const std::vector<float>& input)
{
    auto engine = fsdsp::makeEngine (engineName);
    engine->prepare (vowelprobe::kSampleRate, kBlock, 1);
    engine->setPitchSemitones (pitch);
    engine->setFormantSemitones (formant);

    const fsdsp::PlanarAudio audio (1, input);
    return fsdsp::renderAligned (*engine, audio, kBlock)[0];
}
} // namespace

int main()
{
    const auto input = vowelprobe::makeVowel();
    const double f0In = vowelprobe::estimateF0 (input);
    const double centroidIn = vowelprobe::spectralCentroid (input);

    std::printf ("input: f0 %.1f Hz, spectral centroid %.1f Hz\n\n", f0In, centroidIn);
    std::printf ("%-24s %6s %8s | %9s %7s | %9s %7s\n",
                 "engine", "pitch", "formant", "f0 ratio", "ideal", "formant", "ideal");

    struct Setting { float pitch, formant; };
    const Setting settings[] = { { 0.0f, 0.0f }, { 7.0f, 0.0f }, { -5.0f, 0.0f },
                                 { 0.0f, 5.0f }, { 0.0f, -5.0f }, { 7.0f, 5.0f } };

    for (const std::string engine : { "passthrough", "signalsmith", "signalsmith-coupled" })
    {
        for (const auto& s : settings)
        {
            const auto output = render (engine, s.pitch, s.formant, input);

            std::printf ("%-24s %+6.0f %+8.0f | %9.3f %7.3f | %9.3f %7.3f\n",
                         engine.c_str(), static_cast<double> (s.pitch), static_cast<double> (s.formant),
                         vowelprobe::estimateF0 (output) / f0In,
                         std::pow (2.0, static_cast<double> (s.pitch) / 12.0),
                         vowelprobe::spectralCentroid (output) / centroidIn,
                         std::pow (2.0, static_cast<double> (s.formant) / 12.0));
        }

        std::printf ("\n");
    }

    return 0;
}