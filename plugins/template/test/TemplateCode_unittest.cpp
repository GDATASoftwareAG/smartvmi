#include <TemplateCode.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vmicore/os/ActiveProcessInformation.h>
#include <vmicore_test/io/mock_Logger.h>
#include <vmicore_test/plugins/mock_PluginInterface.h>
#include <vmicore_test/vmi/mock_IntrospectionAPI.h>

using testing::_;
using testing::NiceMock;
using VmiCore::ActiveProcessInformation;
using VmiCore::MockIntrospectionAPI;
using VmiCore::Plugin::MockPluginInterface;

namespace Template
{
    TEST(TemplateCodeTests, doStuffWithProcessStart_activeProcess_readFromDtb)
    {
        // Test setup
        uint64_t processDtb = 0x1337;
        auto processInformation =
            std::make_shared<ActiveProcessInformation>(ActiveProcessInformation{.processUserDtb = processDtb});
        std::shared_ptr<MockIntrospectionAPI> introspectionApi = std::make_shared<MockIntrospectionAPI>();
        std::unique_ptr<NiceMock<MockPluginInterface>> pluginInterface =
            std::make_unique<NiceMock<MockPluginInterface>>();
        ON_CALL(*pluginInterface, newNamedLogger(_))
            .WillByDefault([]() { return std::make_unique<NiceMock<VmiCore::MockLogger>>(); });
        auto templateCode = TemplateCode{pluginInterface.get(), introspectionApi};

        // Test assertion. Peculiarity of gmock: Assertion needs to take place before execution
        EXPECT_CALL(*introspectionApi, read64PA(processDtb)).Times(1);

        // Test execution
        EXPECT_NO_THROW(templateCode.doStuffWithProcessStart(processInformation));
    }
}
