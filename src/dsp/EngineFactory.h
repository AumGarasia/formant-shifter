#pragma once

#include "PitchFormantEngine.h"

#include <memory>
#include <string>
#include <vector>

namespace fsdsp
{

/** 
    Names accepted by makeEngine(), for help text. The two signalsmith engines also
    accept an "@<ms>" suffix that sets the analysis block length, for example
    "signalsmith@60" (10 to 500 ms). 
*/
std::vector<std::string> engineNames();

// Returns nullptr if 'name' is not recognized.
std::unique_ptr<PitchFormantEngine> makeEngine (const std::string& name);

}   // namespace fsdsp