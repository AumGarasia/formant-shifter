#pragma once

#include "PitchFormantEngine.h"

namespace fsdsp {

    class PassthroughEngine : public PitchFormantEngine {

        public:
            void prepare (double sampleRate, int maxBlockSize, int numChannels) override;
            void reset() override;
            void setPitchSemitones (float semitones) override;
            void setFormantSemitones (float semitones) override;
            void process (const float* const* inputs, float* const* outputs, 
                          int numChannels, int numFrames) override;
            int getLatencySamples() const override;
    };
    
}