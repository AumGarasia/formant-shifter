#include "PluginProcessor.h"

#include "dsp/SignalsmithEngine.h"

#include <algorithm>

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", params::createLayout())
{
    pitchParam = apvts.getRawParameterValue (params::pitchId);
    formantParam = apvts.getRawParameterValue (params::formantId);
    mixParam = apvts.getRawParameterValue (params::mixId);
    outputParam = apvts.getRawParameterValue (params::outputId);

    jassert (pitchParam != nullptr && formantParam != nullptr
             && mixParam != nullptr && outputParam != nullptr);
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    preparedChannels = getTotalNumInputChannels();
    preparedMaxBlock = std::max (samplesPerBlock, 1);

    if (preparedChannels < 1)
    {
        core.reset();
        return;
    }

    // Engine defaults: formants independent of pitch (measured with engine_probe) and the
    // library's 120 ms preset blocks. OPEN DECISION: block size / latency.
    core = std::make_unique<fsdsp::EffectCore> (std::make_unique<fsdsp::SignalsmithEngine>());

    core = std::make_unique<fsdsp::EffectCore> (std::make_unique<fsdsp::SignalsmithEngine> (options));
    core->prepare (sampleRate, preparedMaxBlock, preparedChannels);

    // A fresh core knows nothing about the current parameter values. Push them,
    // then snap the ramps so the first block starts exactly at those values.
    appliedPitch = appliedFormant = appliedMix = appliedOutput = kNothingApplied;
    updateCoreParameters();
    core->reset();

    // Latency depends only on the sample rate, so it is identical on every prepare.
    // Never reset it to 0 elsewhere: each change makes the host restart the plugin.
    setLatencySamples (core->getLatencySamples());
}

void PluginProcessor::releaseResources()
{
    core.reset();
    preparedChannels = 0;
    preparedMaxBlock = 0;
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return in == out;   // mono->mono or stereo->stereo only
}

void PluginProcessor::updateCoreParameters()
{
    const float pitch = pitchParam->load();
    const float formant = formantParam->load();
    const float mix = mixParam->load() / 100.0f;
    const float output = outputParam->load();

    if (! juce::exactlyEqual (pitch, appliedPitch))
    {
        core->setPitchSemitones (pitch);
        appliedPitch = pitch;
    }

    if (! juce::exactlyEqual (formant, appliedFormant))
    {
        core->setFormantSemitones (formant);
        appliedFormant = formant;
    }

    if (! juce::exactlyEqual (mix, appliedMix))
    {
        core->setMix (mix);
        appliedMix = mix;
    }

    if (! juce::exactlyEqual (output, appliedOutput))
    {
        core->setOutputGainDb (output);
        appliedOutput = output;
    }
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Clear any output channels that have no matching input.
    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    // Not prepared, or the host changed the layout without re-preparing:
    // pass the audio through untouched instead of risking a bad call.
    if (core == nullptr || numChannels != preparedChannels || numSamples <= 0)
        return;

    updateCoreParameters();
    core->process (buffer.getArrayOfWritePointers(), numChannels, numSamples);
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (const auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (const auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
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