#include "../../../src/os/windows/SystemEventSupervisor.h"
#include "../../config/mock_ConfigInterface.h"
#include "../../io/mock_EventStream.h"
#include "../../io/mock_Logging.h"
#include "../../plugins/mock_PluginSystem.h"
#include "../../vmi/mock_InterruptFactory.h"
#include "../../vmi/mock_LibvmiInterface.h"
#include "mock_ActiveProcessesSupervisor.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::NiceMock;

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
        std::shared_ptr<MockInterruptFactory> interruptFactory = std::make_shared<MockInterruptFactory>();
        std::shared_ptr<MockLogging> logging = std::make_shared<NiceMock<MockLogging>>();
        std::shared_ptr<MockEventStream> eventStream = std::make_shared<MockEventStream>();
        std::shared_ptr<Windows::SystemEventSupervisor> systemEventSupervisor =
            std::make_shared<Windows::SystemEventSupervisor>(vmiInterface,
                                                             pluginSystem,
                                                             activeProcessSupervisor,
                                                             configInterface,
                                                             interruptFactory,
                                                             logging,
                                                             eventStream);
    };

    TEST_F(SystemEventSupervisorFixture, teardown_validState_interruptFactoryTeardownCalled)
    {
        EXPECT_CALL(*interruptFactory, teardown()).Times(1);

        EXPECT_NO_THROW(systemEventSupervisor->teardown());
    }
}
