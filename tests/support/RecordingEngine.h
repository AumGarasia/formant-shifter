#pragma once

#include "dsp/PitchFormantEngine.h"

#include <algorithm>

namespace testsupport
{
    // Does nothing to the audio; records how it was called.
    class RecordingEngine final : public::fsdsp::PitchFormantEngine
    {
    public:
        void prepare (double, int, int) override {};
        void reset() override {};
        void setPitchSemitones (float) override {};
        void setFormantSemitones (float) override {};

        void process (const float* const*, float* const*, int, int numFrames) override
        {
            ++calls;
            totalFrames += numFrames;
            largest = std::max (largest, numFrames);
        }

        int getLatencySamples() const override { return 0; }
        int calls = 0;
        int totalFrames = 0;
        int largest = 0;
    };

} // namespace testsupport