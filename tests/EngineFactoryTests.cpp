#include <catch2/catch_test_macros.hpp>

#include "dsp/EngineFactory.h"

#include <cstdlib>

TEST_CASE ("makeEngine accepts every advertised name", "[factory]")
{
    for (const auto& name : fsdsp::engineNames())
        REQUIRE (fsdsp::makeEngine (name) != nullptr);
}

TEST_CASE ("makeEngine rejects unknown names", "[factory]")
{
    REQUIRE (fsdsp::makeEngine ("nonsense") == nullptr);
    REQUIRE (fsdsp::makeEngine ("") == nullptr);
}

TEST_CASE ("makeEngine parses the block length suffix", "[factory]")
{
    REQUIRE (fsdsp::makeEngine ("signalsmith@60") != nullptr);
    REQUIRE (fsdsp::makeEngine ("signalsmith-coupled@40") != nullptr);

    REQUIRE (fsdsp::makeEngine ("signalsmith@") == nullptr);
    REQUIRE (fsdsp::makeEngine ("signalsmith@abc") == nullptr);
    REQUIRE (fsdsp::makeEngine ("signalsmith@5") == nullptr);        // too short
    REQUIRE (fsdsp::makeEngine ("signalsmith@1000") == nullptr);     // too long
    REQUIRE (fsdsp::makeEngine ("passthrough@60") == nullptr);       // meaningless here
}

TEST_CASE ("The block length scales the latency, and the default is 80 ms", "[factory][signalsmith]")
{
    auto latencyOf = [] (const char* name)
    {
        auto engine = fsdsp::makeEngine (name);
        engine->prepare (44100.0, 512, 1);
        return engine->getLatencySamples();
    };

    const int byDefault = latencyOf ("signalsmith");
    const int eighty = latencyOf ("signalsmith@80");
    const int forty = latencyOf ("signalsmith@40");
    const int oneTwenty = latencyOf ("signalsmith@120");

    REQUIRE (std::abs (byDefault - eighty) <= 2);                         // the shipping default
    REQUIRE (std::abs (forty - eighty / 2) <= eighty / 20);               // roughly proportional (within 5%)
    REQUIRE (std::abs (oneTwenty - eighty * 3 / 2) <= eighty / 20);
}