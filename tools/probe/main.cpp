// Measures how an engine's pitch and formant controls behave on a synthetic vowel.
//   engine_probe formants   independence of pitch and formant controls (default)
//   engine_probe blocks     latency / accuracy / purity at different Signalsmith block lengths
// Run it in a Release build.

#include "VowelProbe.h"

#include "dsp/EngineFactory.h"
#include "dsp/OfflineRenderer.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr int kBlock = 512;

struct Rendered
{
    std::vector<float> audio;
    int latencySamples;
    double seconds;       // wall-clock time spent rendering
};

Rendered render (const std::string& engineName, float pitch, float formant,
                 const std::vector<float>& input)
{
    auto engine = fsdsp::makeEngine (engineName);
    engine->prepare (vowelprobe::kSampleRate, kBlock, 1);
    engine->setPitchSemitones (pitch);
    engine->setFormantSemitones (formant);

    const fsdsp::PlanarAudio audio (1, input);

    const auto start = std::chrono::steady_clock::now();
    auto output = fsdsp::renderAligned (*engine, audio, kBlock);
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;

    return { std::move (output[0]), engine->getLatencySamples(), elapsed.count() };
}

int runFormantTable()
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
            const auto output = render (engine, s.pitch, s.formant, input).audio;

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

int runBlockTable()
{
    constexpr float pitch = 7.0f;
    const double audioSeconds = static_cast<double> (vowelprobe::kLength) / vowelprobe::kSampleRate;

    for (const double f0 : { 80.0, 140.0, 220.0 })
    {
        const auto input = vowelprobe::makeVowel (f0);
        const auto inputSpectrum = vowelprobe::magnitudeSpectrum (input);
        const double f0In = vowelprobe::estimateF0 (input);
        const double centroidIn = vowelprobe::spectralCentroid (inputSpectrum);

        std::printf ("\nvowel on %.0f Hz (input purity %.1f%%), pitch +7, formant 0\n",
                     f0, 100.0 * vowelprobe::harmonicPurity (inputSpectrum, f0In));
        std::printf ("%-18s %10s %9s %9s %9s %10s\n",
                     "engine", "latency ms", "f0 ratio", "formant", "purity %", "x realtime");

        for (const std::string engine : { "signalsmith@120", "signalsmith@80",
                                          "signalsmith@60", "signalsmith@40" })
        {
            const auto r = render (engine, pitch, 0.0f, input);
            const auto spectrum = vowelprobe::magnitudeSpectrum (r.audio);
            const double f0Out = vowelprobe::estimateF0 (r.audio);

            std::printf ("%-18s %10.1f %9.3f %9.3f %9.1f %10.0f\n", engine.c_str(),
                         1000.0 * r.latencySamples / vowelprobe::kSampleRate,
                         f0Out / f0In,
                         vowelprobe::spectralCentroid (spectrum) / centroidIn,
                         100.0 * vowelprobe::harmonicPurity (spectrum, f0Out),
                         audioSeconds / r.seconds);
        }
    }

    std::printf ("\nideal: f0 ratio %.3f, formant 1.000\n", std::pow (2.0, static_cast<double> (pitch) / 12.0));
    return 0;
}
} // namespace

int main (int argc, char* argv[])
{
    const std::string mode = argc > 1 ? argv[1] : "formants";

    if (mode == "formants")
        return runFormantTable();

    if (mode == "blocks")
        return runBlockTable();

    std::fprintf (stderr, "usage: engine_probe [formants|blocks]\n");
    return 1;
}