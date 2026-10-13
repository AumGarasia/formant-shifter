#include "ProcessChunked.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>

namespace fsdsp
{

void processInPlaceChunked (PitchFormantEngine& engine, float* const* channels,
                            int numChannels, int numFrames, int maxChunk)
{
    assert (numChannels >= 1 && numChannels <= kMaxChunkedChannels);
    assert (maxChunk > 0);

    // Release-build safety; never index past our fixed array.
    if (numChannels < 1 || numChannels > kMaxChunkedChannels || maxChunk <= 0)  return;

    std::array<float*, static_cast<std::size_t> (kMaxChunkedChannels) > chunk {};
    const auto channelCount = static_cast<std::size_t> (numChannels);

    for (int start = 0; start < numFrames; start += maxChunk)
    {
        const int frames = std::min (maxChunk, numFrames - start);

        for (std::size_t ch = 0; ch < channelCount; ++ch)
            chunk[ch] = channels[ch] + start;
        
        engine.process(chunk.data(), chunk.data(), numChannels, frames);
    }
}

} // namespace fsdsps