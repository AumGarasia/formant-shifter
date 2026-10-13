#include "SignalsmithEngine.h"

#include "signalsmith-stretch.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

namespace fsdsp
{

struct SignalsmithEngine::Impl
{
    explicit Impl (SignalsmithOptions o) : options (o) {}

    SignalsmithOptions options;

    // Fixed seed: the library otherwise seeds from std::random_device, which would
    // make output differ from run to run.
    signalsmith::stretch::SignalsmithStretch<float> stretch { 12345L };

    int numChannels = 0;
    int maxBlockSize = 0;
    int latency = 0;
    float pitchSemitones = 0.0f;
    float formantSemitones = 0.0f;

    std::vector<float> scratch;           // numChannels * maxBlockSize, channel-major
    std::vector<float*> scratchChannels;  // one pointer per channel into `scratch`

    void applyParameters()
    {
        stretch.setTransposeSemitones (pitchSemitones);
        stretch.setFormantSemitones (formantSemitones, options.compensatePitch);
    }
};

SignalsmithEngine::SignalsmithEngine (SignalsmithOptions options)
    : impl (std::make_unique<Impl> (options))
{
}

SignalsmithEngine::~SignalsmithEngine() = default;

void SignalsmithEngine::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    assert (sampleRate > 0.0 && maxBlockSize > 0 && numChannels > 0);

    auto& s = *impl;
    s.numChannels = numChannels;
    s.maxBlockSize = maxBlockSize;

    s.stretch.presetDefault (numChannels, static_cast<float> (sampleRate), s.options.splitComputation);

    const auto channels = static_cast<std::size_t> (numChannels);
    const auto block = static_cast<std::size_t> (maxBlockSize);

    s.scratch.assign (channels * block, 0.0f);
    s.scratchChannels.resize (channels);

    for (std::size_t ch = 0; ch < channels; ++ch)
        s.scratchChannels[ch] = s.scratch.data() + ch * block;

    s.latency = s.stretch.inputLatency() + s.stretch.outputLatency();
    s.applyParameters();   // configuring may have reset the library's settings
}

void SignalsmithEngine::reset()
{
    impl->stretch.reset();
}

void SignalsmithEngine::setPitchSemitones (float semitones)
{
    impl->pitchSemitones = semitones;
    impl->stretch.setTransposeSemitones (semitones);
}

void SignalsmithEngine::setFormantSemitones (float semitones)
{
    impl->formantSemitones = semitones;
    impl->stretch.setFormantSemitones (semitones, impl->options.compensatePitch);
}

void SignalsmithEngine::process (const float* const* inputs, float* const* outputs,
                                 int numChannels, int numFrames)
{
    auto& s = *impl;

    assert (numChannels == s.numChannels);
    assert (numFrames <= s.maxBlockSize);

    if (numFrames <= 0)
        return;

    // The library does not allow its input and output buffers to be the same, but our
    // interface does. Always copy the input aside, which also covers partial overlap.
    for (int ch = 0; ch < numChannels; ++ch)
        std::copy_n (inputs[ch], numFrames, s.scratchChannels[static_cast<std::size_t> (ch)]);

    s.stretch.process (s.scratchChannels.data(), numFrames, outputs, numFrames);
}

int SignalsmithEngine::getLatencySamples() const
{
    return impl->latency;
}

} // namespace fsdsp