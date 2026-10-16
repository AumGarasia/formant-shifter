#include <catch2/catch_test_macros.hpp>

#include "ui/ValueFormat.h"

using valueFormat::formatValue;

TEST_CASE ("formatValue prints ordinary values", "[format]")
{
    REQUIRE (formatValue (5.0, 2) == "5.00");
    REQUIRE (formatValue (-3.5, 2) == "-3.50");
    REQUIRE (formatValue (24.0, 2) == "24.00");
    REQUIRE (formatValue (-6.0, 1) == "-6.0");
    REQUIRE (formatValue (100.0, 0) == "100");
}

TEST_CASE ("formatValue never prints a negative zero", "[format]")
{
    REQUIRE (formatValue (0.0, 2) == "0.00");
    REQUIRE (formatValue (-0.0, 2) == "0.00");
    REQUIRE (formatValue (-0.0004, 2) == "0.00");     // rounds to zero
    REQUIRE (formatValue (0.0004, 2) == "0.00");
    REQUIRE (formatValue (-0.04, 1) == "0.0");
    REQUIRE (formatValue (-0.4, 0) == "0");
}

TEST_CASE ("formatValue keeps the sign of values that do not round to zero", "[format]")
{
    REQUIRE (formatValue (-0.01, 2) == "-0.01");
    REQUIRE (formatValue (0.01, 2) == "0.01");
}