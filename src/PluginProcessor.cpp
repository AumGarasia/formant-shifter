#include "PluginProcessor.h"

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                      .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void PluginProcessor::prepareToPlay (double, int) {}
void PluginProcessor::releaseResources() {}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if ( out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    
    return in == out;   // mono -> mono or stereo -> stereo only
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Pass-through: input and output share the buffer, so there's nothing to do.
    // Clear any output channels that have no matching input.
    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

// Entry point JUCE calls to create the plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}