#ifndef VMICORE_MOCK_INTERRUPTEVENTSUPERVISOR_H
#define VMICORE_MOCK_INTERRUPTEVENTSUPERVISOR_H

#include <gmock/gmock.h>
#include <vmi/InterruptEventSupervisor.h>

namespace VmiCore
{
    class MockInterruptEventSupervisor : public IInterruptEventSupervisor
    {
      public:
        MOCK_METHOD(void, initialize, (), (override));

        MOCK_METHOD(void, teardown, (), (override));

        MOCK_METHOD(std::shared_ptr<IBreakpoint>,
                    createBreakpoint,
                    (uint64_t,
                     std::shared_ptr<const ActiveProcessInformation>,
                     const std::function<BpResponse(IInterruptEvent&)>&,
                     bool),
                    (override));

        MOCK_METHOD(void, deleteBreakpoint, (IBreakpoint*), (override));
    };
}

#endif // VMICORE_MOCK_INTERRUPTEVENTSUPERVISOR_H
