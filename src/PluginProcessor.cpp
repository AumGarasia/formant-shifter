#include "PluginProcessor.h"

#include "dsp/ProcessChunked.h"
#include "dsp/SignalsmithEngine.h"

#include <algorithm>

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", params::createLayout())
{
    pitchParam = apvts.getRawParameterValue (params::pitchID);
    formantParam = apvts.getRawParameterValue (params::formantId);

    jassert (pitchParam != nullptr && formantParam != nullptr);
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    preparedChannels = std::min (getTotalNumInputChannels(), fsdsp::kMaxChunkedChannels);
    preparedMaxBlock = std::max (samplesPerBlock, 1);

    if (preparedChannels < 1)
    {
        engine.reset();
        return;
    }

    // OPEN DECISIONS (settle by ear in the offline harness):
    //   - compensatePitch: how formant shifting combines with pitch shifting
    //   - block size / latency: the preset uses 120 ms blocks
    fsdsp::SignalsmithOptions options;
    options.compensatePitch = false;

    engine = std::make_unique<fsdsp::SignalsmithEngine> (options);
    engine->prepare (sampleRate, preparedMaxBlock, preparedChannels);

    // A fresh engine knows nothing about the current parameter values.
    appliedPitch = std::numeric_limits<float>::quiet_NaN();
    appliedFormant = std::numeric_limits<float>::quiet_NaN();
    updateEngineParameters();

    // Latency depends only on the sample rate, so it is identical on every prepare.
    // Never reset it to 0 elsewhere: each change makes the host restart the plugin.
    setLatencySamples (engine->getLatencySamples());
}

void PluginProcessor::releaseResources()
{
    engine.reset();
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

void PluginProcessor::updateEngineParameters()
{
    const float pitch = pitchParam->load();
    const float formant = formantParam->load();

    if (! juce::exactlyEqual (pitch, appliedPitch))
    {
        engine->setPitchSemitones (pitch);
        appliedPitch = pitch;
    }

    if (! juce::exactlyEqual (formant, appliedFormant))
    {
        engine->setFormantSemitones (formant);
        appliedFormant = formant;
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
    if (engine == nullptr || numChannels != preparedChannels || numSamples <= 0)
        return;

    updateEngineParameters();

    fsdsp::processInPlaceChunked (*engine, buffer.getArrayOfWritePointers(),
                                  numChannels, numSamples, preparedMaxBlock);
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