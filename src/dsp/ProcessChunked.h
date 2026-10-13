#pragma once

#include "PitchFormantEngine.h"

namespace fsdsp
{

// Largest channel count processInPlaceChunked() supports.
constexpr int kMaxChunkedChannels = 8;

/** 
    Runs numFrames of audio through 'engine', in place, in chunks of at most
    maxChunk frames. 'channels' holds numChannels pointers.

    Hosts may deliver blocks larger than the size announced at prepare time;
    this guarantees the engine never sees more than maxChunk frames at once.

    Real-time safe; no allocation; no locks.
*/
void processInPlaceChunked (PitchFormantEngine& engine, float* const* channels,
                            int numChannels, int numFrames, int maxChunk);

}   // namespace fsdsp