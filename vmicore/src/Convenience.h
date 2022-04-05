#ifndef VMICORE_CONVENIENCE_H
#define VMICORE_CONVENIENCE_H

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

namespace Convenience
{
    template <typename T> std::string intToHex(T i)
    {
        std::stringstream stream;
        stream << "0x" << std::hex << (uint64_t)i;
        return stream.str();
    }

    template <typename T> std::string intToDec(T i)
    {
        std::stringstream stream;
        stream << std::dec << (uint64_t)i;
        return stream.str();
    }
}

#endif // VMICORE_CONVENIENCE_H
