#pragma once

#include "PitchFormantEngine.h"

#include <memory>
#include <string>
#include <vector>

namespace fsdsp
{

// Names accepted by makeEngine(), for help text.
std::vector<std::string> engineNames();

// Returns nullptr if 'name' is not recognized.
std::unique_ptr<PitchFormantEngine> makeEngine (const std::string& name);

}   // namespace fsdsp