#pragma once

#include "LinearRamp.h"
#include "PitchFormantEngine.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace fsdsp
{
/**
    The whole audio effect, free of any plugin framework: pitch/formant engine,
    a latency-matched dry path, dry/wet mix and output gain with click-free ramps.
    The plugin is thin glue around this class.

    Threading: prepare() and reset() are callof off the audio thread. The setters and
    process() are called from the audio thread only, never concurrently.
 */
class EffectCore
{
public:
    explicit EffectCore (std::unique_ptr<PitchFormantEngine> engingToOwn);

    // Allocates everything. process() accepts any block size afterwards.
    void prepare (double sampleRate, int maxBlockSize, int numChannels);

    // Clears all audio state and snaps the ramps to their targets.
    void reset();

    void setPitchSemitones (float semitones);
    void setFormantSemitones (float semitones);
    void setMix (float mix);                    // 0 = complete dry signal, 1 = complete wet signa;
    void setOutputGainDb (float decibels);

    // In place, numChannels must match prepare(); any numFrames >= 0 is fine.
    void process (float* const* channels, int channelCount, int numFrames);

    // Delay between input and output; identical for every mix setting.
    int getLatencySamples() const;

private:
    void processChunk (float* const* channels, int numFrames);
    
    std::unique_ptr<PitchFormantEngine> engine;

    int rampSamples = 0;
    int maxBlock = 0;
    int numChannels = 0;
    int latency = 0;
    bool prepared = false;

    float pitch = 0.0f;
    float formant = 0.0f;
    LinearRamp mixRamp;
    LinearRamp gainRamp;

    // Dry path: one ring buffer per channel, 'latency' samples long.
    std::vector<std::vector<float>> dryLines;
    std::size_t dryPos = 0;
    std::vector<std::vector<float>> dryScratch;     // Delayed dry for the current chunk
    std::vector<float*> chunkPtrs;                  // per-channel pointers into the caller's buffer.
};

}   // namespace fsdsp