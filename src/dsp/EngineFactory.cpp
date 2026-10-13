#include "EngineFactory.h"

#include "PassthroughEngine.h"
#include "SignalsmithEngine.h"

namespace fsdsp
{

std::vector<std::string> engineNames()
{
    return { "passthrough", "signalsmith", "signalsmith-compensated" };
}

std::unique_ptr<PitchFormantEngine> makeEngine (const std::string& name)
{
    if (name ==  "passthrough") return std::make_unique<PassthroughEngine>();

    if (name == "signalsmith")  return std::make_unique<SignalsmithEngine>();

    if (name == "signalsmith-compensated")
    {
        SignalsmithOptions options;
        options.compensatePitch = true;
        return std::make_unique<SignalsmithEngine> (options);
    }

    return nullptr;
}

}   // namespace fsdsp