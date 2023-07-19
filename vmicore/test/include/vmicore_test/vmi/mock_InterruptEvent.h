#ifndef VMICORE_MOCK_INTERRUPTEVENT_H
#define VMICORE_MOCK_INTERRUPTEVENT_H

#include <gmock/gmock.h>
#include <vmicore/vmi/events/IInterruptEvent.h>

namespace VmiCore
{
    class MockInterruptEvent : public IInterruptEvent
    {
      public:
        MOCK_METHOD(uint64_t, getRax, (), (const override));

        MOCK_METHOD(uint64_t, getRbx, (), (const override));

        MOCK_METHOD(uint64_t, getRcx, (), (const override));

        MOCK_METHOD(uint64_t, getRdx, (), (const override));

        MOCK_METHOD(uint64_t, getRdi, (), (const override));

        MOCK_METHOD(uint64_t, getR8, (), (const override));

        MOCK_METHOD(uint64_t, getR9, (), (const override));

        MOCK_METHOD(uint64_t, getRip, (), (const override));

        MOCK_METHOD(uint64_t, getRsp, (), (const override));

        MOCK_METHOD(uint64_t, getCr3, (), (const override));

        MOCK_METHOD(uint64_t, getGs, (), (const override));

        MOCK_METHOD(addr_t, getGla, (), (const override));

        MOCK_METHOD(addr_t, getGfn, (), (const override));

        MOCK_METHOD(addr_t, getOffset, (), (const override));
    };
}

#endif // VMICORE_MOCK_INTERRUPTEVENT_H
