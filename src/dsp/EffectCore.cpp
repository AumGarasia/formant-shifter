#include "EffectCore.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <utility>

namespace fsdsp
{

namespace
{
constexpr double kRampSeconds = 0.02;

float dbToGain (float decibels)
{
    return std::pow (10.0f, decibels / 20.0f);
}
} // namespace

EffectCore::EffectCore (std::unique_ptr<PitchFormantEngine> engineToOwn)
    : engine (std::move (engineToOwn))
{
    assert (engine != nullptr);

    mixRamp.snapTo (1.0f);    // default: fully wet
    gainRamp.snapTo (1.0f);   // default: 0 dB
}

void EffectCore::prepare (double sampleRate, int maxBlockSize, int channels)
{
    assert (sampleRate > 0.0 && maxBlockSize > 0 && channels > 0);

    maxBlock = std::max (maxBlockSize, 1);
    numChannels = std::max (channels, 1);
    rampSamples = std::max (1, static_cast<int> (std::lround (sampleRate * kRampSeconds)));

    engine->prepare (sampleRate, maxBlock, numChannels);
    latency = std::max (0, engine->getLatencySamples());

    const auto channelCount = static_cast<std::size_t> (numChannels);
    dryLines.assign (channelCount, std::vector<float> (static_cast<std::size_t> (latency), 0.0f));
    dryScratch.assign (channelCount, std::vector<float> (static_cast<std::size_t> (maxBlock), 0.0f));
    chunkPtrs.assign (channelCount, nullptr);
    dryPos = 0;

    prepared = true;
    applyToEngine();
    reset();
}

void EffectCore::reset()
{
    if (! prepared)
        return;

    engine->reset();

    for (auto& line : dryLines)
        std::fill (line.begin(), line.end(), 0.0f);

    dryPos = 0;
    mixRamp.snapTo (mixRamp.getTarget());
    gainRamp.snapTo (gainRamp.getTarget());
}

float EffectCore::effectiveFormantSemitones() const
{
    const float total = formant + (formantFollowsPitch ? pitch : 0.0f);
    return std::clamp (total, -kMaxFormantShiftSemitones, kMaxFormantShiftSemitones);
}

void EffectCore::applyToEngine()
{
    if (! prepared)
        return;

    engine->setPitchSemitones (pitch);
    engine->setFormantSemitones (effectiveFormantSemitones());
}

void EffectCore::setPitchSemitones (float semitones)
{
    pitch = semitones;
    applyToEngine();
}

void EffectCore::setFormantSemitones (float semitones)
{
    formant = semitones;
    applyToEngine();
}

void EffectCore::setFormantFollowsPitch (bool follows)
{
    formantFollowsPitch = follows;
    applyToEngine();
}

void EffectCore::setMix (float mix)
{
    mixRamp.setTarget (std::clamp (mix, 0.0f, 1.0f), prepared ? rampSamples : 0);
}

void EffectCore::setOutputGainDb (float decibels)
{
    gainRamp.setTarget (dbToGain (decibels), prepared ? rampSamples : 0);
}

int EffectCore::getLatencySamples() const
{
    return latency;
}

void EffectCore::process (float* const* channels, int channelCount, int numFrames)
{
    assert (prepared && channelCount == numChannels);

    if (! prepared || channelCount != numChannels)
        return;

    for (int start = 0; start < numFrames; start += maxBlock)
    {
        const int frames = std::min (maxBlock, numFrames - start);

        for (std::size_t ch = 0; ch < chunkPtrs.size(); ++ch)
            chunkPtrs[ch] = channels[ch] + start;

        processChunk (chunkPtrs.data(), frames);
    }
}

void EffectCore::processChunk (float* const* channels, int numFrames)
{
    const auto frames = static_cast<std::size_t> (numFrames);

    // 1. Capture the dry input, and take out the dry signal delayed by `latency`.
    for (std::size_t ch = 0; ch < dryLines.size(); ++ch)
    {
        auto& scratch = dryScratch[ch];

        if (latency == 0)
        {
            std::copy_n (channels[ch], frames, scratch.begin());
            continue;
        }

        auto& line = dryLines[ch];
        std::size_t pos = dryPos;

        for (std::size_t i = 0; i < frames; ++i)
        {
            scratch[i] = line[pos];          // read first, then overwrite with the new input
            line[pos] = channels[ch][i];

            if (++pos == line.size())
                pos = 0;
        }
    }

    if (latency > 0)
        dryPos = (dryPos + frames) % dryLines.front().size();

    // 2. Wet: the engine processes the chunk in place.
    engine->process (channels, channels, numChannels, numFrames);

    // 3. Mix and apply output gain; one ramp step per sample, shared by all channels.
    for (std::size_t i = 0; i < frames; ++i)
    {
        const float mix = mixRamp.next();
        const float gain = gainRamp.next();
        const float dryAmount = 1.0f - mix;

        for (std::size_t ch = 0; ch < dryScratch.size(); ++ch)
            channels[ch][i] = (dryScratch[ch][i] * dryAmount + channels[ch][i] * mix) * gain;
    }
}

} // namespace fsdsp