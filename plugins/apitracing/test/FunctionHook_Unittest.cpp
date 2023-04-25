#include "../src/lib/FunctionHook.h"
#include "mock_Extractor.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vmicore_test/io/mock_Logger.h>
#include <vmicore_test/plugins/mock_PluginInterface.h>
#include <vmicore_test/vmi/mock_InterruptEvent.h>
#include <vmicore_test/vmi/mock_IntrospectionAPI.h>

using testing::_; // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
using testing::NiceMock;
using testing::Return;
using VmiCore::MockInterruptEvent;
using VmiCore::MockIntrospectionAPI;
using VmiCore::Plugin::MockPluginInterface;

namespace ApiTracing
{
    class FunctionHookTestFixture : public testing::Test
    {
      protected:
        std::unique_ptr<MockPluginInterface> pluginInterface = std::make_unique<NiceMock<MockPluginInterface>>();
        std::shared_ptr<MockIntrospectionAPI> introspectionAPI = std::make_shared<NiceMock<MockIntrospectionAPI>>();
        std::shared_ptr<MockExtractor> extractor = std::make_shared<NiceMock<MockExtractor>>();
        std::shared_ptr<MockInterruptEvent> interruptEvent = std::make_shared<NiceMock<MockInterruptEvent>>();

        static constexpr VmiCore::addr_t testModuleBase = 0x420;
        static constexpr VmiCore::addr_t testDtb = 0x1337;

        void SetUp() override
        {
            ON_CALL(*interruptEvent, getCr3()).WillByDefault(Return(testDtb));
            ON_CALL(*pluginInterface, newNamedLogger(_))
                .WillByDefault([]() { return std::make_unique<NiceMock<VmiCore::MockLogger>>(); });
        }
    };

    TEST_F(FunctionHookTestFixture, hookFunction_validFunction_noThrow)
    {
        FunctionHook functionHook{"TestModule",
                                  "TestFunction",
                                  extractor,
                                  introspectionAPI,
                                  std::make_shared<std::vector<ParameterInformation>>(
                                      std::vector<ParameterInformation>{{.name = "TestParameter"}}),
                                  pluginInterface.get()};

        ASSERT_NO_THROW(functionHook.hookFunction(testModuleBase, testDtb));
    }

    TEST_F(FunctionHookTestFixture, hookFunction_validFunction_createsHook)
    {
        FunctionHook functionHook{"TestModule",
                                  "TestFunction",
                                  extractor,
                                  introspectionAPI,
                                  std::make_shared<std::vector<ParameterInformation>>(
                                      std::vector<ParameterInformation>{{.name = "TestParameter"}}),
                                  pluginInterface.get()};
        EXPECT_CALL(*introspectionAPI, translateUserlandSymbolToVA).Times(1);
        EXPECT_CALL(*pluginInterface, createBreakpoint).Times(1);

        functionHook.hookFunction(testModuleBase, testDtb);
    }

    TEST_F(FunctionHookTestFixture, hookCallBack_functionHookWithParameters_extractsParameters)
    {
        FunctionHook functionHook{"TestModule",
                                  "TestFunction",
                                  extractor,
                                  introspectionAPI,
                                  std::make_shared<std::vector<ParameterInformation>>(
                                      std::vector<ParameterInformation>{{.name = "TestParameter"}}),
                                  pluginInterface.get()};

        functionHook.hookFunction(testModuleBase, testDtb);
        EXPECT_CALL(*extractor, extractParameters).Times(1);

        functionHook.hookCallback(*interruptEvent);
    }
}
