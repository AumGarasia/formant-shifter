#pragma once

#include "PitchFormantEngine.h"

#include <memory>

namespace fsdsp
{

struct SignalsmithOptions
{
    /** Spreads FFT work evenly across calls, at the cost of one extra interval of latency. */
    bool splitComputation = false;

    /** Forwarded to the library's formant setter. We'll settle the right value by ear. */
    bool compensatePitch = false;
};

/** PitchFormantEngine backed by Signalsmith Stretch (MIT). */
class SignalsmithEngine final : public PitchFormantEngine
{
public:
    explicit SignalsmithEngine (SignalsmithOptions options = {});
    ~SignalsmithEngine() override;

    void prepare (double sampleRate, int maxBlockSize, int numChannels) override;
    void reset() override;
    void setPitchSemitones (float semitones) override;
    void setFormantSemitones (float semitones) override;
    void process (const float* const* inputs, float* const* outputs,
                  int numChannels, int numFrames) override;
    int getLatencySamples() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace fsdsp