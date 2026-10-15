#pragma once

#include <algorithm>

namespace fsdsp
{
/**
    Largest formant shift ever sent to an engine. Linked mode can add pitch (up to 24)
    to formant (up to 12), so the sum is clamped to this conservative limit.
*/
inline constexpr float kMaxFormantShiftSemitones = 24.0f;

struct EffectiveFormant
{
    float semitones;    // What the engine actually receives
    bool clamped;       // True if the limit changed the value
};

// The formant shift the engine receives for the given control settings.
inline EffectiveFormant effectiveFormant (float pitchSemitones, float formantSemitones, bool followsPitch)
{
    const float total = formantSemitones + (followsPitch ? pitchSemitones : 0.0f);
    const float limited = std::clamp (total, -kMaxFormantShiftSemitones, kMaxFormantShiftSemitones);

    // Written without != because the warning flags reject float equality.
    return { limited, limited < total || limited > total };
}

}   // namespace fsdsp