#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace testsignals
{

/** Deterministic noise in [-0.5, 0.5], different for each seed. */
inline std::vector<float> makeNoise (std::size_t length, std::uint32_t seed)
{
    std::vector<float> noise (length);
    std::uint32_t state = seed;

    for (auto& x : noise)
    {
        state = state * 1664525u + 1013904223u;
        x = (static_cast<float> (state >> 8) / 8388608.0f - 1.0f) * 0.5f;
    }

    return noise;
}

} // namespace testsignals