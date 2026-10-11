#include <juce_audio_formats/juce_audio_formats.h>

#include <iostream>
#include <memory>

int main (int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: offline_render <input.wav/>\n";
        return 1;
    }

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    const auto input = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (input));

    if (reader == nullptr)
    {
        std::cerr << "Failed to open input file: " << argv[1] << "\n";
        return 1;
    }

    std::cout << "Input file: " << input.getFullPathName().toStdString() << '\n'
                << "Sample rate: " << reader->sampleRate << '\n'
                << "Channels: " << reader->numChannels << '\n'
                << "Length (samples): " << reader->lengthInSamples << " samples ("
                << static_cast<double> (reader->lengthInSamples) / reader->sampleRate << " s)\n"
                << "Bit depth: " << reader->bitsPerSample << (reader->usesFloatingPointData ? " (float)" : " (int)") << '\n';
    
    return 0;
}