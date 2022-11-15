#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>
#include <vmicore/os/PagingDefinitions.h>

namespace InMemoryScanner
{
    template <typename T> std::string intToHex(T i)
    {
        std::stringstream stream;
        stream << "0x" << std::hex << (uint64_t)i;
        return stream.str();
    }

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

    inline std::size_t bytesToNumberOfPages(std::size_t size)
    {
        return (size + VmiCore::PagingDefinitions::pageSizeInBytes - 1) / VmiCore::PagingDefinitions::pageSizeInBytes;
    }
}
