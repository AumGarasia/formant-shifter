#include "EngineFactory.h"

#include "PassthroughEngine.h"
#include "SignalsmithEngine.h"

#include <cmath>
#include <cstdlib>
#include <optional>

namespace fsdsp
{

namespace
{

// Splits "signalsmith@60" into the base name and an optional block length in ms.
// Returns false if there is a suffix that isn't a number between 10 and 500.
bool splitName (const std::string& name, std::string& base, std::optional<double>& blockMs)
{
    const auto at = name.find ('@');
    base = name.substr (0, at);
    blockMs.reset();

    if (at == std::string::npos)
        return true;

    const std::string suffix = name.substr (at + 1);
    char* end = nullptr;
    const double value = std::strtod (suffix.c_str(), &end);

    if (suffix.empty() || *end != '\0' || ! std::isfinite (value) || value < 10.0 || value > 500.0)
        return false;

    blockMs = value;
    return true;
}

} // namespace

std::vector<std::string> engineNames()
{
    return { "passthrough", "signalsmith", "signalsmith-coupled" };
}

std::unique_ptr<PitchFormantEngine> makeEngine (const std::string& name)
{
    std::string base;
    std::optional<double> blockMs;

    if (! splitName (name, base, blockMs))
        return nullptr;

    if (base == "passthrough")
    {
        if (blockMs.has_value())
            return nullptr;                                // a block length means nothing here

        return std::make_unique<PassthroughEngine>();
    }

    if (base == "signalsmith" || base == "signalsmith-coupled")
    {
        SignalsmithOptions options;
        options.compensatePitch = (base == "signalsmith"); // "-coupled": formants follow the pitch shift

        if (blockMs.has_value())
            options.blockMs = *blockMs;                    // no suffix: keep the 80 ms default

        return std::make_unique<SignalsmithEngine> (options);
    }

    return nullptr;
}

} // namespace fsdsp