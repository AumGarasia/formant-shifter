#pragma once

#include "dsp/PitchFormantEngine.h"

#include <cstddef>
#include <algorithm>
#include <vector>

namespace testsupport
{

/** Delays its input by exactly `delay` samples and scales it by `gain`.
    Reports the delay as its latency. */
class DelayEngine final : public fsdsp::PitchFormantEngine
{
public:
    explicit DelayEngine (int delaySamples, float outputGain = 1.0f)
        : delay (delaySamples), gain (outputGain) {}

    void prepare (double, int, int numChannels) override
    {
        lines.assign (static_cast<std::size_t> (numChannels),
                      std::vector<float> (static_cast<std::size_t> (delay), 0.0f));
        pos = 0;
    }

    void reset() override
    {
        for (auto& line : lines)
            std::fill (line.begin(), line.end(), 0.0f);

        pos = 0;
    }

    void setPitchSemitones (float) override {}
    void setFormantSemitones (float) override {}

    void process (const float* const* inputs, float* const* outputs,
                  int numChannels, int numFrames) override
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& line = lines[static_cast<std::size_t> (ch)];
            std::size_t p = pos;

            for (int i = 0; i < numFrames; ++i)
            {
                const float x = inputs[ch][i];   // read first: in-place safe

                if (line.empty())
                {
                    outputs[ch][i] = gain * x;
                    continue;
                }

                outputs[ch][i] = gain * line[p];
                line[p] = x;
                p = (p + 1) % line.size();
            }
        }

        if (delay > 0)
            pos = (pos + static_cast<std::size_t> (numFrames)) % static_cast<std::size_t> (delay);
    }

    int getLatencySamples() const override { return delay; }

private:
    int delay;
    float gain;
    std::size_t pos = 0;
    std::vector<std::vector<float>> lines;
};

} // namespace testsupport