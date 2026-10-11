#pragma once

#include "PitchFormantEngine.h"

#include <vector>

namespace fsdsp
{
// Planar audio: one vector of samples per channel, all the same length.
using PlanarAudio = std::vector<std::vector<float>>;

/** Runs `input` through `engine` in blocks of at most `blockSize` frames and
    returns the result aligned with the input (same length, latency removed).

    Preconditions:
      - engine.prepare() was called with maxBlockSize >= blockSize and
        numChannels == input.size(), and parameters are already set.
      - input has at least one channel, and all channels have equal length.
      - blockSize > 0.

    The engine is reset() first, so results never depend on previous use.
*/
PlanarAudio renderAligned (PitchFormantEngine& engine, const PlanarAudio& input, int blockSize);

} // namespace fsdsp