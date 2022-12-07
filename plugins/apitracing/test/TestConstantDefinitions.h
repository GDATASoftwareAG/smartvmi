#ifndef APITRACING_TESTCONSTANTDEFINITIONS_H
#define APITRACING_TESTCONSTANTDEFINITIONS_H
#include <cstdint>
namespace ApiTracing
{
    class TestConstantDefinitions
    {
      public:
        static constexpr uint8_t oneByte = 0x1;
        static constexpr uint8_t twoBytes = 0x2;
        static constexpr uint8_t fourBytes = 0x4;
        static constexpr uint8_t eightBytes = 0x8;
    };
}
#endif // APITRACING_TESTCONSTANTDEFINITIONS_H
