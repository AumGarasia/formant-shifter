#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "dsp/LinearRamp.h"

using Catch::Approx;

TEST_CASE ("LinearRamp reaches its target in exactly the requested number of samples", "[ramp]")
{
    fsdsp::LinearRamp ramp;
    ramp.snapTo (1.0f);
    ramp.setTarget (3.0f, 4);

    REQUIRE (ramp.next() == Approx (1.5f));
    REQUIRE (ramp.next() == Approx (2.0f));
    REQUIRE (ramp.next() == Approx (2.5f));
    REQUIRE (ramp.next() == Approx (3.0f));
    REQUIRE_FALSE (ramp.isRamping());
    REQUIRE (ramp.next() == Approx (3.0f));
}

TEST_CASE ("LinearRamp continues from where it is when retargeted", "[ramp]")
{
    fsdsp::LinearRamp ramp;
    ramp.snapTo (0.0f);
    ramp.setTarget (10.0f, 10);

    for (int i = 0; i < 5; ++i)
        ramp.next();                // Now at 5
    
    ramp.setTarget (0.0f, 5);       // Heads back down, 1 per sample

    REQUIRE (ramp.next() == Approx(4.0f));
}

TEST_CASE ("LinearRamp with zero-length ramp snaps", "[ramp]")
{
    fsdsp::LinearRamp ramp;
    ramp.snapTo (1.0f);
    ramp.setTarget (2.0f, 0);

    REQUIRE_FALSE (ramp.isRamping());
    REQUIRE (ramp.next() == Approx(2.0f));
}

TEST_CASE ("LinearRamp ignores a repeated target instead of restarting", "[ramp]")
{
    fsdsp::LinearRamp ramp;
    ramp.snapTo (1.0f);
    ramp.setTarget (2.0f, 4);

    REQUIRE (ramp.next() == Approx(1.25f));

    ramp.setTarget (2.0f, 4);       // same target; mus not restart

    REQUIRE (ramp.next() == Approx(1.5f));
}