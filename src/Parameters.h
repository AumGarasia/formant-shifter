#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace params
{
// IDs are permanent: hosts store automation and saved sets by ID.
inline constexpr const char* pitchId = "Pitch";
inline constexpr const char* formantId = "Formant";
inline constexpr const char* mixId = "Mix";
inline constexpr const char* outputId = "Output";

inline constexpr float pitchRangeSemitones = 24.0f;
inline constexpr float formantRangeSemitones = 12.0f;
inline constexpr float outputMinDb = -24.0f;
inline constexpr float outputMaxDb = 12.0f;

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using Param = juce::AudioParameterFloat;
    using Attrs = juce::AudioParameterFloatAttributes;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<Param> (juce::ParameterID { pitchId, 1}, "Pitch",
                                         juce::NormalisableRange<float> (-pitchRangeSemitones, pitchRangeSemitones, 0.01f),
                                         0.0f, Attrs().withLabel ("st")));
    
    layout.add (std::make_unique<Param> (juce::ParameterID { formantId, 1}, "Formant",
                                         juce::NormalisableRange<float> (-formantRangeSemitones, formantRangeSemitones, 0.01f),
                                         0.0f, Attrs().withLabel ("st")));
    
    layout.add (std::make_unique<Param> (juce::ParameterID { mixId, 1}, "Mix",
                                         juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                         0.0f, Attrs().withLabel ("%")));
    
    layout.add (std::make_unique<Param> (juce::ParameterID { outputId, 1}, "Output",
                                         juce::NormalisableRange<float> (outputMinDb, outputMaxDb, 0.01f),
                                         0.0f, Attrs().withLabel ("dB")));

    return layout;
}

}   // namespace params