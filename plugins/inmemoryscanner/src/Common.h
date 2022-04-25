#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

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
