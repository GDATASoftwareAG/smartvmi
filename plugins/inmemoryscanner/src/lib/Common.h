#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>
#include <vmicore/filename.h>

#define INMEMORY_LOGGER_NAME std::string("InMemory_").append(FILENAME_STEM)

namespace InMemoryScanner
{
    struct Match
    {
        std::string matchName;
        int64_t position;
    };

    struct Rule
    {
        std::string ruleName;
        std::string ruleNamespace;
        std::vector<Match> matches;
    };
}
