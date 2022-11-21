#ifndef VMICORE_MOCK_INTERRUPTEVENT_H
#define VMICORE_MOCK_INTERRUPTEVENT_H

#include <gmock/gmock.h>
#include <vmicore/vmi/events/IInterruptEvent.h>

namespace VmiCore
{
    class MockInterruptEvent : public IInterruptEvent
    {
      public:
        MOCK_METHOD(uint64_t, getRax, (), (override));

        MOCK_METHOD(uint64_t, getRbx, (), (override));

        MOCK_METHOD(uint64_t, getRcx, (), (override));

        MOCK_METHOD(uint64_t, getRdx, (), (override));

        MOCK_METHOD(uint64_t, getRdi, (), (override));

        MOCK_METHOD(uint64_t, getR8, (), (override));

        MOCK_METHOD(uint64_t, getR9, (), (override));

        MOCK_METHOD(uint64_t, getRip, (), (override));

        MOCK_METHOD(uint64_t, getCr3, (), (override));

        MOCK_METHOD(addr_t, getGla, (), (override));

        MOCK_METHOD(addr_t, getGfn, (), (override));

        MOCK_METHOD(addr_t, getOffset, (), (override));
    };
}

#endif // VMICORE_MOCK_INTERRUPTEVENT_H
