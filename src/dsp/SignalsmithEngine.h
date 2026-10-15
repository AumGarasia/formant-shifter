#pragma once

#include "PitchFormantEngine.h"

#include <memory>
#include <optional>

namespace fsdsp
{

struct SignalsmithOptions
{
    /** Spreads FFT work evenly across calls, at the cost of one extra interval of latency. */
    bool splitComputation = false;

    /** Measured with engine_probe: with this on, the formant control is independent of
        the pitch control (formants stay put when only pitch moves). With it off,
        formants follow the pitch shift. */
    bool compensatePitch = true;

    /** Analysis block length in milliseconds. Unset means the library's default preset
        (120 ms). Latency is roughly proportional to this. Shorter blocks resolve low
        notes less well, so they can smear and warble. The interval is a quarter of the block. */
    std::optional<double> blockMs;
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