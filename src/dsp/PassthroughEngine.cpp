#include "PassthroughEngine.h"

#include <cstddef>
#include <cstring>

namespace fsdsp {

    void PassthroughEngine::prepare (double, int, int) {}
    void PassthroughEngine::reset() {}
    void PassthroughEngine::setPitchSemitones (float) {}
    void PassthroughEngine::setFormantSemitones (float) {}
    int PassthroughEngine::getLatencySamples() const { return 0; }

    void PassthroughEngine::process (const float* const* inputs, float* const* outputs, 
                                     int numChannels, int numFrames)
    {
        if (numFrames <= 0) return;

        const auto bytes = static_cast<std::size_t> (numFrames) * sizeof (float);

        for (int ch = 0; ch < numChannels; ++ch)
            if (inputs[ch] != outputs[ch])      // In-place: nothing to copy
                std::memcpy (outputs[ch], inputs[ch], bytes);
    }
}