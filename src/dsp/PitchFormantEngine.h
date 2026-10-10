#pragma once

namespace fsdsp {
    /**
     * A pitch and formant engine for audio processing.
        Threading: prepare() and reset() are called off the audio thread; everything 
        else is called on the audio thread only, never concurrently.

        Real-time rule: process() must not allocate memory, lock mutexes or throw exceptions. 
        It must not call any functions that do so either. It must not call any functions that 
        may block, such as sleep() or wait().
    */

    class PitchFormantEngine
    {
        public:
            virtual ~PitchFormantEngine() = default;

            // Allocates everythin. maxBlockSize is the largest numFrames process() will see.
            virtual void prepare (double sampleRate, int maxBlockSize, int numChannels) = 0;

            // Clear internal state (delay lines etc.) without reallocating memory.
            virtual void reset() = 0;

            virtual void setPitchSemitones (float semitones) = 0;
            virtual void setFormantSemitones (float semitones) = 0;

            // Inputs/Outputs are arrays of numChannels pointers to arrays of numFrames floats.
            // Each channel holds numFrames samples of audio. The input and output arrays may be the same.
            virtual void process (const float* const* inputs, float* const* outputs, 
                                  int numChannels, int numFrames) = 0;
            
            // Delay in samples between input and output. Valid after prepare().
            virtual int getLatencySamples() const = 0;
    };
    
}