#include "../io/mock_Logging.h"
#include "mock_LibvmiInterface.h"
#include "vmi/RegisterEventSupervisor.h"
#include <gtest/gtest.h>
#include <vmicore_test/io/mock_Logger.h>

using testing::_;
using testing::AnyNumber;
using testing::ByMove;
using testing::NiceMock;
using testing::Ref;
using testing::Return;
using testing::SaveArg;

namespace VmiCore
{
    namespace
    {
        constexpr vmi_instance* vmiInstanceStub = nullptr;
    }

    class ContextSwitchHandlerFixture : public testing::Test
    {
      protected:
        std::shared_ptr<MockLibvmiInterface> vmiInterface = std::make_shared<MockLibvmiInterface>();
        std::shared_ptr<NiceMock<MockLogging>> mockLogging = std::make_shared<NiceMock<MockLogging>>();
        std::shared_ptr<IRegisterEventSupervisor> contextSwitchHandler;
        vmi_event_t* internalContextSwitchEvent = nullptr;

        void SetUp() override
        {
            ON_CALL(*mockLogging, newNamedLogger(_))
                .WillByDefault([](std::string_view) { return std::make_unique<MockLogger>(); });
            ON_CALL(*vmiInterface, registerEvent(_))
                .WillByDefault(
                    [&internalContextSwitchEvent = internalContextSwitchEvent](vmi_event_t& event)
                    {
                        if (event.type == VMI_EVENT_REGISTER)
                        {
                            internalContextSwitchEvent = &event;
                        }
                    });

            contextSwitchHandler = std::make_shared<RegisterEventSupervisor>(vmiInterface, mockLogging);
        }
    };

    TEST_F(ContextSwitchHandlerFixture, defaultContextSwitchCallback_noCallbackRegistered_doesNotThrow)
    {
        contextSwitchHandler->setContextSwitchCallback([](vmi_event_t*) {});

        EXPECT_NO_THROW(RegisterEventSupervisor::_defaultRegisterCallback(vmiInstanceStub, internalContextSwitchEvent));
    }

    TEST_F(ContextSwitchHandlerFixture, defaultContextSwitchCallback_validCallback_doesNotThrow)
    {
        contextSwitchHandler->setContextSwitchCallback([](vmi_event_t*) {});

        EXPECT_NO_THROW(RegisterEventSupervisor::_defaultRegisterCallback(vmiInstanceStub, internalContextSwitchEvent));
    }

    TEST_F(ContextSwitchHandlerFixture, setContextSwitchCallback_callbackAlreadyRegistered_throws)
    {
        contextSwitchHandler->setContextSwitchCallback([](vmi_event_t*) {});

        EXPECT_ANY_THROW(contextSwitchHandler->setContextSwitchCallback([](vmi_event_t*) {}));
    }
}
