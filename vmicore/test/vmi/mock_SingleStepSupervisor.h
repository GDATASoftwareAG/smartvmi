#include "../../src/vmi/SingleStepSupervisor.h"
#include <gmock/gmock.h>

class MockSingleStepSupervisor : public ISingleStepSupervisor
{
  public:
    MOCK_METHOD(void, initializeSingleStepEvents, (), (override));
    MOCK_METHOD(void, teardown, (), (override));
    MOCK_METHOD(void, setSingleStepCallback, (uint, const std::function<void(vmi_event_t*)>&), (override));
};
