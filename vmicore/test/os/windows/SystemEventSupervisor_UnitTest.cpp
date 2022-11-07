#include "../../../src/os/windows/SystemEventSupervisor.h"
#include "../../config/mock_ConfigInterface.h"
#include "../../io/grpc/mock_GRPCLogger.h"
#include "../../io/mock_EventStream.h"
#include "../../io/mock_Logging.h"
#include "../../plugins/mock_PluginSystem.h"
#include "../../vmi/mock_InterruptEventSupervisor.h"
#include "../../vmi/mock_LibvmiInterface.h"
#include "mock_ActiveProcessesSupervisor.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vmicore/test/vmi/mock_Breakpoint.h>

using testing::_;
using testing::NiceMock;
using testing::Return;

namespace VmiCore
{
    class SystemEventSupervisorFixture : public testing::Test
    {
      protected:
        std::shared_ptr<MockLibvmiInterface> vmiInterface = std::make_shared<NiceMock<MockLibvmiInterface>>();
        std::shared_ptr<MockPluginSystem> pluginSystem = std::make_shared<NiceMock<MockPluginSystem>>();
        std::shared_ptr<MockActiveProcessesSupervisor> activeProcessSupervisor =
            std::make_shared<NiceMock<MockActiveProcessesSupervisor>>();
        std::shared_ptr<MockConfigInterface> configInterface = std::make_shared<NiceMock<MockConfigInterface>>();
        std::shared_ptr<MockInterruptEventSupervisor> interruptEventSupervisor =
            std::make_shared<MockInterruptEventSupervisor>();
        std::shared_ptr<MockLogging> logging = std::make_shared<NiceMock<MockLogging>>();
        std::shared_ptr<MockEventStream> eventStream = std::make_shared<MockEventStream>();
        std::shared_ptr<Windows::SystemEventSupervisor> systemEventSupervisor;

        void SetUp() override
        {
            ON_CALL(*logging, newNamedLogger(_))
                .WillByDefault([](const std::string_view&) { return std::make_unique<MockGRPCLogger>(); });
            systemEventSupervisor = std::make_shared<Windows::SystemEventSupervisor>(vmiInterface,
                                                                                     pluginSystem,
                                                                                     activeProcessSupervisor,
                                                                                     configInterface,
                                                                                     interruptEventSupervisor,
                                                                                     logging,
                                                                                     eventStream);
        }
    };

    TEST_F(SystemEventSupervisorFixture, teardown_validState_interruptEventSupervisorTeardownCalled)
    {
        ON_CALL(*interruptEventSupervisor, createBreakpoint(_, _, _))
            .WillByDefault([](uint64_t, uint64_t, const std::function<BpResponse(IInterruptEvent&)>&)
                           { return std::make_shared<NiceMock<MockBreakpoint>>(); });
        systemEventSupervisor->initialize();

        EXPECT_CALL(*interruptEventSupervisor, teardown()).Times(1);

        EXPECT_NO_THROW(systemEventSupervisor->teardown());
    }
}
