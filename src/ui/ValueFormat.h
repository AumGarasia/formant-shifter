#pragma once

#include <cmath>
#include <cstdio>
#include <string>


namespace valueFormat
{
/**
    Formats a value for the LED readouts: "5.00", "-3.50", "0.00". A value that rounds to
    zero prints as plain zero, never as "-0.00".
*/
inline std::string formatValue (double value, int decimals)
{
    const double scale = std::pow (10.0, decimals);
    const double rounded = std::round (value * scale) / scale;

    char buffer[32];

    // Written without == because the warning flags reject equality.
    if (rounded > 0.0 || rounded < 0.0)
        std::snprintf (buffer, sizeof buffer, "%.*f", decimals, rounded);
    else
        std::snprintf (buffer, sizeof buffer, "%.*f", decimals, 0.0);

    return buffer;
}

}   // namespace valueFormat