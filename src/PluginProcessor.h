#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Parameters.h"
#include "dsp/EffectCore.h"

#include <atomic>
#include <limits>
#include <memory>

class PluginProcessor final : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    void updateCoreParameters();

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float>* pitchParam = nullptr;
    std::atomic<float>* formantParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* outputParam = nullptr;

    std::unique_ptr<fsdsp::EffectCore> core;
    int preparedChannels = 0;
    int preparedMaxBlock = 0;

    // Last values pushed to the core (NaN = "nothing applied yet").
    static constexpr float kNothingApplied = std::numeric_limits<float>::quiet_NaN();
    float appliedPitch = kNothingApplied;
    float appliedFormant = kNothingApplied;
    float appliedMix = kNothingApplied;
    float appliedOutput = kNothingApplied;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};