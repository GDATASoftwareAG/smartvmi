#ifndef VMICORE_MOCK_SINGLESTEPSUPERVISOR_H
#define VMICORE_MOCK_SINGLESTEPSUPERVISOR_H

#include "../../src/vmi/SingleStepSupervisor.h"
#include <gmock/gmock.h>

namespace VmiCore
{
    class MockSingleStepSupervisor : public ISingleStepSupervisor
    {
      public:
        MOCK_METHOD(void, initializeSingleStepEvents, (), (override));
        MOCK_METHOD(void, teardown, (), (override));
        MOCK_METHOD(void,
                    setSingleStepCallback,
                    (uint, const std::function<void(vmi_event_t*)>&, uint64_t),
                    (override));
    };
}

#endif // VMICORE_MOCK_SINGLESTEPSUPERVISOR_H
