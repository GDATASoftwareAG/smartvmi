#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>
#include <vmicore/filename.h>
#include <vmicore/os/PagingDefinitions.h>

#define INMEMORY_LOGGER_NAME std::string("InMemory_").append(FILENAME_STEM)

namespace InMemoryScanner
{
    struct Match
    {
        std::string matchName;
        int64_t position;

        bool operator==(const Match& rhs) const = default;
    };

    struct Rule
    {
        std::string ruleName;
        std::string ruleNamespace;
        std::vector<Match> matches;

        bool operator==(const Rule& rhs) const = default;
    };

    inline std::size_t bytesToNumberOfPages(std::size_t size)
    {
        return (size + VmiCore::PagingDefinitions::pageSizeInBytes - 1) / VmiCore::PagingDefinitions::pageSizeInBytes;
    }
}
