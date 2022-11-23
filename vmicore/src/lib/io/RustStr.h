#ifndef VMICORE_RUSTSTR_H
#define VMICORE_RUSTSTR_H

#include <cxxbridge/rust/cxx.h>
#include <string_view>

namespace VmiCore
{
    inline ::rust::Str toRustStr(std::string_view stringView)
    {
        return {stringView.data(), stringView.size()};
    }
}

#endif // VMICORE_RUSTSTR_H
