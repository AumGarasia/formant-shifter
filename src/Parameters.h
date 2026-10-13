#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace params
{
// IDs are permanent: hosts store automation and saved sets by ID.
inline constexpr const char* pitchID = "Pitch";
inline constexpr const char* formantId = "Formant";

inline constexpr float pitchRangeSemitones = 24.0f;
inline constexpr float formantRangeSemitones = 12.0f;

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using Param = juce::AudioParameterFloat;
    using Attrs = juce::AudioParameterFloatAttributes;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<Param> (juce::ParameterID { pitchID, 1}, "Pitch",
                                         juce::NormalisableRange<float> (-pitchRangeSemitones, pitchRangeSemitones, 0.01f),
                                         0.0f, Attrs().withLabel ("st")));
    
    layout.add (std::make_unique<Param> (juce::ParameterID { formantId, 1}, "Formant",
                                         juce::NormalisableRange<float> (-formantRangeSemitones, formantRangeSemitones, 0.01f),
                                         0.0f, Attrs().withLabel ("st")));
    
    return layout;
}

}   // namespace params