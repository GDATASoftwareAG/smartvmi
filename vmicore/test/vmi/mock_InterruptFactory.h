#include "../../src/vmi/InterruptFactory.h"
#include <gmock/gmock.h>

namespace VmiCore
{
    class MockInterruptFactory : public IInterruptFactory
    {
      public:
        MOCK_METHOD(void, initialize, (), (override));

        MOCK_METHOD(void, teardown, (), (override));

        MOCK_METHOD(std::shared_ptr<InterruptEvent>,
                    createInterruptEvent,
                    (const std::string& interruptName,
                     uint64_t targetVA,
                     uint64_t systemCr3,
                     std::function<InterruptEvent::InterruptResponse(InterruptEvent&)> callbackFunction),
                    (override));
    };
}
