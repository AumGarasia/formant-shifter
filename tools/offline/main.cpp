#include <juce_audio_formats/juce_audio_formats.h>

#include "dsp/PassthroughEngine.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string>

namespace
{
constexpr int kBlockSize = 512;
constexpr float kMaxPitchSemitones = 24.0f;
constexpr float kMaxFormantSemitones = 12.0f;

struct Options
{
    juce::File input;
    juce::File output;
    float pitchSemitones = 0.0f;
    float formantSemitones = 0.0f;
};

bool parseFloat (const char* text, float maxAbs, float& out)
{
    char* end = nullptr;
    const float value = std::strtof (text, &end);

    if (end == text || *end != '\0' || ! std::isfinite (value) || std::fabs (value) > maxAbs)
        return false;

    out = value;
    return true;
}

std::optional<Options> parseArgs (int argc, char* argv[])
{
    if (argc < 2)
        return std::nullopt;

    Options opts;
    opts.input = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);

    int i = 2;
    while (i < argc)
    {
        const std::string flag = argv[i++];                 // consume the flag

        if (flag != "--pitch" && flag != "--formant" && flag != "--output")
        {
            std::cerr << "error: unknown option '" << flag << "'\n";
            return std::nullopt;
        }

        if (i >= argc)
        {
            std::cerr << "error: " << flag << " needs a value\n";
            return std::nullopt;
        }

        const char* value = argv[i++];                      // consume the value

        if (flag == "--output")
        {
            opts.output = juce::File::getCurrentWorkingDirectory().getChildFile (value);
        }
        else
        {
            const bool isPitch = (flag == "--pitch");
            const float limit = isPitch ? kMaxPitchSemitones : kMaxFormantSemitones;
            float& target = isPitch ? opts.pitchSemitones : opts.formantSemitones;

            if (! parseFloat (value, limit, target))
            {
                std::cerr << "error: " << flag << " needs a number between -" << limit
                          << " and " << limit << "\n";
                return std::nullopt;
            }
        }
    }

    if (opts.output == juce::File())
        opts.output = opts.input.getSiblingFile (opts.input.getFileNameWithoutExtension() + "_out.wav");

    if (opts.output == opts.input)
    {
        std::cerr << "error: output would overwrite the input file\n";
        return std::nullopt;
    }

    if (! opts.output.hasFileExtension ("wav"))
    {
        std::cerr << "error: output must be a .wav file\n";
        return std::nullopt;
    }

    if (! opts.output.getParentDirectory().isDirectory())
    {
        std::cerr << "error: output directory does not exist\n";
        return std::nullopt;
    }

    return opts;
}

float maxAbsDifference (const juce::AudioBuffer<float>& a, int aStart,
                        const juce::AudioBuffer<float>& b, int bStart, int numSamples)
{
    float worst = 0.0f;

    for (int ch = 0; ch < a.getNumChannels(); ++ch)
    {
        const float* x = a.getReadPointer (ch, aStart);
        const float* y = b.getReadPointer (ch, bStart);

        for (int i = 0; i < numSamples; ++i)
            worst = std::max (worst, std::fabs (x[i] - y[i]));
    }

    return worst;
}
} // namespace

int main (int argc, char* argv[])
{
    const auto opts = parseArgs (argc, argv);

    if (! opts.has_value())
    {
        std::cerr << "usage: offline_render <input.wav> [--pitch semitones] "
                     "[--formant semitones] [--output out.wav]\n";
        return 1;
    }

    // --- Read the input ---
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (opts->input));

    if (reader == nullptr)
    {
        std::cerr << "error: could not read '" << opts->input.getFullPathName().toStdString() << "'\n";
        return 1;
    }

    if (reader->lengthInSamples <= 0 || reader->lengthInSamples > std::numeric_limits<int>::max())
    {
        std::cerr << "error: file is empty or too long for this tool\n";
        return 1;
    }

    const double sampleRate = reader->sampleRate;
    const int numChannels = static_cast<int> (reader->numChannels);
    const int numSamples = static_cast<int> (reader->lengthInSamples);

    juce::AudioBuffer<float> input (numChannels, numSamples);

    if (! reader->read (&input, 0, numSamples, 0, true, true))
    {
        std::cerr << "error: failed to read audio data\n";
        return 1;
    }

    // --- Engine setup ---
    std::unique_ptr<fsdsp::PitchFormantEngine> engine = std::make_unique<fsdsp::PassthroughEngine>();
    engine->prepare (sampleRate, kBlockSize, numChannels);
    engine->setPitchSemitones (opts->pitchSemitones);
    engine->setFormantSemitones (opts->formantSemitones);

    // The engine delays its output by `latency` samples, so feed it that many
    // extra frames of silence to flush the tail, then skip the first `latency`
    // output samples to line the result up with the input.
    const int latency = engine->getLatencySamples();
    const int totalFrames = numSamples + latency;

    juce::AudioBuffer<float> padded (numChannels, totalFrames);
    padded.clear();
    for (int ch = 0; ch < numChannels; ++ch)
        padded.copyFrom (ch, 0, input, ch, 0, numSamples);

    juce::AudioBuffer<float> rendered (numChannels, totalFrames);
    rendered.clear();

    juce::HeapBlock<const float*> inPtrs (static_cast<std::size_t> (numChannels));
    juce::HeapBlock<float*> outPtrs (static_cast<std::size_t> (numChannels));

    // --- Process in blocks ---
    const double startMs = juce::Time::getMillisecondCounterHiRes();

    for (int start = 0; start < totalFrames; start += kBlockSize)
    {
        const int frames = std::min (kBlockSize, totalFrames - start);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            inPtrs[ch] = padded.getReadPointer (ch, start);
            outPtrs[ch] = rendered.getWritePointer (ch, start);
        }

        engine->process (inPtrs.get(), outPtrs.get(), numChannels, frames);
    }

    const double elapsedMs = juce::Time::getMillisecondCounterHiRes() - startMs;
    const double audioMs = 1000.0 * numSamples / sampleRate;

    // --- Write the aligned result as 32-bit float WAV ---
    // FileOutputStream appends to an existing file, so remove any old output first.
    if (opts->output.existsAsFile() && ! opts->output.deleteFile())
    {
        std::cerr << "error: could not replace '" << opts->output.getFullPathName().toStdString() << "'\n";
        return 1;
    }

    std::unique_ptr<juce::OutputStream> stream = opts->output.createOutputStream();

    if (stream == nullptr)
    {
        std::cerr << "error: could not open '" << opts->output.getFullPathName().toStdString()
                  << "' for writing\n";
        return 1;
    }

    juce::WavAudioFormat wav;
    const auto writerOptions = juce::AudioFormatWriterOptions{}
                                   .withSampleRate (sampleRate)
                                   .withNumChannels (numChannels)
                                   .withBitsPerSample (32)
                                   .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);

    auto writer = wav.createWriterFor (stream, writerOptions);

    if (writer == nullptr)
    {
        std::cerr << "error: could not create a WAV writer\n";
        return 1;
    }

    // Skip the first `latency` samples so the output lines up with the input.
    if (! writer->writeFromAudioSampleBuffer (rendered, latency, numSamples))
    {
        std::cerr << "error: failed while writing audio data\n";
        return 1;
    }

    writer.reset(); // finalises the WAV header and closes the file

    // Read the file back to verify it.
    std::unique_ptr<juce::AudioFormatReader> check (formats.createReaderFor (opts->output));

    if (check == nullptr || check->lengthInSamples != numSamples
        || static_cast<int> (check->numChannels) != numChannels)
    {
        std::cerr << "error: output file failed verification\n";
        return 1;
    }

    // --- Report ---
    std::cout << "input:       " << opts->input.getFullPathName().toStdString() << '\n'
              << "output:      " << opts->output.getFullPathName().toStdString() << '\n'
              << "format:      " << sampleRate << " Hz, " << numChannels << " ch, "
              << numSamples << " samples (" << audioMs / 1000.0 << " s)\n"
              << "settings:    pitch " << opts->pitchSemitones << " st, formant "
              << opts->formantSemitones << " st\n"
              << "latency:     " << latency << " samples\n"
              << "input peak:  " << input.getMagnitude (0, numSamples) << '\n'
              << "output peak: " << rendered.getMagnitude (latency, numSamples) << '\n'
              << "max |diff| vs input: "
              << maxAbsDifference (input, 0, rendered, latency, numSamples) << '\n';

    if (elapsedMs > 0.0)
        std::cout << "speed:       " << audioMs / elapsedMs << "x real time\n";

    return 0;
}