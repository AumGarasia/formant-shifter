#include "OfflineRenderer.h"

#include <algorithm>
#include <cassert>
#include <cstddef>

namespace fsdsp
{

PlanarAudio renderAligned (PitchFormantEngine& engine, const PlanarAudio& input, int blockSize)
{
    assert (blockSize > 0);
    assert (! input.empty());
    assert (std::all_of (input.begin(), input.end(),
                         [&] (const auto& ch) { return ch.size() == input.front().size(); }));

    const std::size_t numChannels = input.size();
    const std::size_t numSamples = input.front().size();
    const std::size_t step = static_cast<std::size_t> (blockSize);

    engine.reset();

    // The engine's output lags its input by 'latency' samples. Feed it that many
    // extra frames of silence so the tail gets flushed, then droup the first
    // 'latency' output samples to line the result up with the input.
    const std::size_t latency = static_cast<std::size_t> (std::max (0, engine.getLatencySamples()));
    const std::size_t totalFrames = numSamples + latency;

    PlanarAudio padded (numChannels, std::vector<float> (totalFrames, 0.0f));
    PlanarAudio rendered (numChannels, std::vector<float> (totalFrames, 0.0f));

    for (std::size_t ch = 0; ch < numChannels; ++ch)
        std::copy (input[ch].begin(), input[ch].end(), padded[ch].begin());
    
    std::vector<const float*> inputPtrs (numChannels);
    std::vector<float*> outputPtrs (numChannels);

    for (std::size_t start = 0; start < totalFrames; start += step)
    {
        const std::size_t numFrames = std::min (step, totalFrames - start);

        for (std::size_t ch = 0; ch < numChannels; ++ch)
        {
            inputPtrs[ch] = padded[ch].data() + start;
            outputPtrs[ch] = rendered[ch].data() + start;
        }

        engine.process (inputPtrs.data(), outputPtrs.data(),
                        static_cast<int> (numChannels), static_cast<int> (numFrames));
    }

    PlanarAudio aligned (numChannels);

    for (std::size_t ch = 0; ch < numChannels; ++ch)
        aligned[ch].assign (rendered[ch].begin() + static_cast<std::ptrdiff_t> (latency), rendered[ch].end());

    return aligned;
}

}   // namespace fsdsp