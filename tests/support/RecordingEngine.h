#pragma once

#include "dsp/PitchFormantEngine.h"

#include <algorithm>

namespace testsupport
{
    // Does nothing to the audio; records how it was called.
    class RecordingEngine final : public::fsdsp::PitchFormantEngine
    {
    public:
        void prepare (double, int, int) override { ++prepareCalls; };
        void reset() override { ++resetCalls; };
        void setPitchSemitones (float semitones) override { lastPitch = semitones; };
        void setFormantSemitones (float semitones) override { lastFormant = semitones; };

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
        int prepareCalls = 0;
        int resetCalls = 0;
        float lastPitch = 0.0f;
        float lastFormant = 0.0f;
    };

} // namespace testsupport