#ifndef APITRACING_CONSTANTDEFINITIONS_H
#define APITRACING_CONSTANTDEFINITIONS_H

#include <cstdint>

namespace ApiTracing::ConstantDefinitions
{
    constexpr uint8_t stackParameterOffsetX64 = 0x28;
    constexpr uint8_t stackParameterOffsetX86 = 0x4;
    constexpr uint8_t maxRegisterParameterCount = 4;
    constexpr uint8_t x64AddressWidth = 64;
    constexpr uint8_t x86AddressWidth = 32;
    constexpr uint8_t byteSize = 8;
}
#endif // APITRACING_CONSTANTDEFINITIONS_H
