#pragma once

namespace fsdsp
{

// Moves linearly from the current value to a target value over a fixed number of samples.
class LinearRamp
{
public:
    // Jumps straight to 'value' cancelling any ramp in progress
    void snapTo (float value)
    {
        current = value;
        target = value;
        step = 0.0f;
        remaining = 0;
    }

    // Starts a ramp from current value to new target value 'newTarget' lasting 'rampSample's
    void setTarget (float newTarget, int rampSamples)
    {
        // Unchanged target: leave any ramp in-progress alone
        if (! (newTarget < target) && ! (newTarget > target))   return;

        target = newTarget;

        if (rampSamples <= 0)
        {
            snapTo ( newTarget );
            return;
        }

        remaining = rampSamples;
        step = ( target - current ) / static_cast<float> (rampSamples);
    }

    // Advances by one sample and returns a new value
    float next()
    {
        if (remaining > 0)
        {
            --remaining;
            current = (remaining == 0) ? target : current + step;
        }

        return current;
    }

    bool isRamping()    const { return remaining > 0; }
    float getTarget()   const { return target; }

private:
    float current = 0.0f;
    float target = 0.0f;
    float step = 0.0f;
    int remaining = 0;
};

}   // namespace fsdsp