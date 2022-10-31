#ifndef VMICORE_MOCK_BREAKPOINT_H
#define VMICORE_MOCK_BREAKPOINT_H

#include <gmock/gmock.h>
#include <vmicore/vmi/IBreakpoint.h>

namespace VmiCore
{
    class MockBreakpoint : public IBreakpoint
    {
      public:
        MOCK_METHOD(addr_t, getTargetPA, (), (override));

        MOCK_METHOD(void, remove, (), (override));
    };
}

#endif // VMICORE_MOCK_BREAKPOINT_H
