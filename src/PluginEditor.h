#pragma once

#include "PluginProcessor.h"
#include "ui/RetroLookAndFeel.h"

#include <array>
#include <atomic>
#include <limits>

class PluginEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    PluginProcessor& pluginProcessor;
    RetroLookAndFeel lookAndFeel;

    // Order matters: attachments must be destroyed before the controls they drive.
    juce::Slider pitchSlider, formantSlider, mixSlider, outputSlider;
    juce::ToggleButton linkButton { "LINK" };

    SliderAttachment pitchAttachment, formantAttachment, mixAttachment, outputAttachment;
    ButtonAttachment linkAttachment;

    // Raw parameter values, read by the timer to drive the effective-formant display.
    std::atomic<float>* pitchValue = nullptr;
    std::atomic<float>* formantValue = nullptr;
    std::atomic<float>* linkValue = nullptr;

    float shownFormant = std::numeric_limits<float>::quiet_NaN();
    bool shownClamped = false;
    bool shownLink = false;

    std::array<juce::Rectangle<int>, 4> columns;
    juce::Rectangle<int> displayBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};