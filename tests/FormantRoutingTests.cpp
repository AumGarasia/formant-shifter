#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "dsp/FormantRouting.h"

using Catch::Approx;

TEST_CASE ("effectiveFormant ignores the pitch when not linked", "[routing]")
{
    const auto e = fsdsp::effectiveFormant (10.0f, 2.0f, false);

    REQUIRE (e.semitones == Approx (2.0f));
    REQUIRE_FALSE (e.clamped);
}

TEST_CASE ("effectiveFormant adds the pitch when linked", "[routing]")
{
    const auto e = fsdsp::effectiveFormant (5.0f, 2.0f, true);

    REQUIRE (e.semitones == Approx (7.0f));
    REQUIRE_FALSE (e.clamped);
}

TEST_CASE ("effectiveFormant clamps at the limit and says so", "[routing]")
{
    const auto high = fsdsp::effectiveFormant (24.0f, 12.0f, true);
    REQUIRE (high.semitones == Approx (24.0f));
    REQUIRE (high.clamped);

    const auto low = fsdsp::effectiveFormant (-24.0f, -12.0f, true);
    REQUIRE (low.semitones == Approx (-24.0f));
    REQUIRE (low.clamped);
}

TEST_CASE ("effectiveFormant exactly at the limit is not reported as clamped", "[routing]")
{
    const auto e = fsdsp::effectiveFormant (24.0f, 0.0f, true);

    REQUIRE (e.semitones == Approx (24.0f));
    REQUIRE_FALSE (e.clamped);
}

TEST_CASE ("An unlinked formant at its own maximum is never clamped", "[routing]")
{
    const auto e = fsdsp::effectiveFormant (24.0f, 12.0f, false);

    REQUIRE (e.semitones == Approx (12.0f));
    REQUIRE_FALSE (e.clamped);
}