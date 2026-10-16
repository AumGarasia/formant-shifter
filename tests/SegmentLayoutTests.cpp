#include <catch2/catch_test_macros.hpp>

#include "ui/SegmentLayout.h"

#include <string>
#include <vector>

namespace
{
// Renders the cells as text: a blank cell is a space, a lit decimal point follows its digit.
std::string show (const std::vector<segmentlayout::Cell>& cells)
{
    std::string s;

    for (const auto& cell : cells)
    {
        s += cell.glyph;

        if (cell.dot)
            s += '.';
    }

    return s;
}
}

TEST_CASE ("layout right-aligns values in the cells", "[segments]")
{
    REQUIRE (show (segmentlayout::layout ("0.00", 5)) == "  0.00");
    REQUIRE (show (segmentlayout::layout ("0", 3)) == "  0");
    REQUIRE (show (segmentlayout::layout ("-3.50", 5)) == " -3.50");
}

TEST_CASE ("layout fits the widest values of every readout", "[segments]")
{
    REQUIRE (show (segmentlayout::layout ("-24.00", 5)) == "-24.00");   // pitch
    REQUIRE (show (segmentlayout::layout ("24.00", 5)) == " 24.00");
    REQUIRE (show (segmentlayout::layout ("100", 3)) == "100");         // mix
    REQUIRE (show (segmentlayout::layout ("-24.0", 4)) == "-24.0");     // output
}

TEST_CASE ("layout ignores a stray unit or suffix", "[segments]")
{
    REQUIRE (show (segmentlayout::layout ("0.00ST", 5)) == "  0.00");
    REQUIRE (show (segmentlayout::layout ("24.00ST", 5)) == " 24.00");
    REQUIRE (show (segmentlayout::layout ("100%", 3)) == "100");
    REQUIRE (show (segmentlayout::layout ("-24.0DB", 4)) == "-24.0");
}

TEST_CASE ("layout shows dashes instead of truncating", "[segments]")
{
    REQUIRE (show (segmentlayout::layout ("123456", 5)) == "-----");
}

TEST_CASE ("layout always returns exactly the requested number of cells", "[segments]")
{
    REQUIRE (segmentlayout::layout ("", 4).size() == 4);
    REQUIRE (segmentlayout::layout ("5", 4).size() == 4);
    REQUIRE (segmentlayout::layout ("12.34", 4).size() == 4);
    REQUIRE (segmentlayout::layout ("99999", 4).size() == 4);
}