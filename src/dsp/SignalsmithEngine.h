#pragma once

#include "PitchFormantEngine.h"

#include <memory>

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

    /** Analysis block length in milliseconds; the interval is a quarter of it. Latency is
    roughly proportional to this. 80 ms was chosen from measurements (engine_probe
    "blocks") and listening: no measurable loss against the library's 120 ms preset,
    even on an 80 Hz voice, and slightly less phasing, for 40 ms less delay. At 60 ms
    low voices start to smear, and at 40 ms they fall apart. */
    double blockMs = 80.0;
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