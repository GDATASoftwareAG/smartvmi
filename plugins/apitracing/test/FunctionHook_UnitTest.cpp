#include "../src/lib/FunctionHook.h"
#include "mock_Extractor.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vmicore/vmi/IBreakpoint.h>
#include <vmicore_test/io/mock_Logger.h>
#include <vmicore_test/os/mock_MemoryRegionExtractor.h>
#include <vmicore_test/plugins/mock_PluginInterface.h>
#include <vmicore_test/vmi/mock_InterruptEvent.h>
#include <vmicore_test/vmi/mock_IntrospectionAPI.h>

using testing::_; // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
using testing::NiceMock;
using testing::Return;
using VmiCore::ActiveProcessInformation;
using VmiCore::addr_t;
using VmiCore::MemoryRegion;
using VmiCore::MockInterruptEvent;
using VmiCore::MockIntrospectionAPI;
using VmiCore::Plugin::MockPluginInterface;

namespace ApiTracing
{
    namespace
    {
        constexpr addr_t tracedProcessDtb = 0x1338000;
        constexpr addr_t tracedProcessUserDtb = 0x1337000;
        constexpr addr_t pcidValue = 1234;
        constexpr std::string_view testModuleName = "TestModule";
        constexpr std::string_view testModuleFunctionName = "TestFunction";
        constexpr addr_t testModuleBase = 0x420;
        constexpr addr_t testModuleFunctionAddress = testModuleBase + 0x123;
    }

    std::shared_ptr<const ActiveProcessInformation> createProcessInformation(addr_t dtb, addr_t userDtb)
    {
        return std::make_shared<ActiveProcessInformation>(0,
                                                          dtb,
                                                          userDtb,
                                                          0,
                                                          0,
                                                          std::string(""),
                                                          std::make_unique<std::string>(""),
                                                          std::make_unique<std::string>(""),
                                                          std::make_unique<VmiCore::MockMemoryRegionExtractor>(),
                                                          true);
    }

    class FunctionHookTestFixture : public testing::Test
    {
      protected:
        std::unique_ptr<MockPluginInterface> pluginInterface = std::make_unique<NiceMock<MockPluginInterface>>();
        std::shared_ptr<MockIntrospectionAPI> introspectionAPI = std::make_shared<NiceMock<MockIntrospectionAPI>>();
        std::shared_ptr<MockExtractor> extractor = std::make_shared<NiceMock<MockExtractor>>();
        std::shared_ptr<MockInterruptEvent> interruptEvent = std::make_shared<NiceMock<MockInterruptEvent>>();

        void SetUp() override
        {
            ON_CALL(*interruptEvent, getCr3()).WillByDefault(Return(tracedProcessUserDtb + pcidValue));
            ON_CALL(*pluginInterface, newNamedLogger(_))
                .WillByDefault([]() { return std::make_unique<NiceMock<VmiCore::MockLogger>>(); });
            ON_CALL(*introspectionAPI,
                    translateUserlandSymbolToVA(testModuleBase, tracedProcessDtb, std::string(testModuleFunctionName)))
                .WillByDefault(Return(testModuleFunctionAddress));
        }
    };

    TEST_F(FunctionHookTestFixture, hookFunction_validFunction_noThrow)
    {
        FunctionHook functionHook{std::string(testModuleName),
                                  std::string(testModuleFunctionName),
                                  extractor,
                                  introspectionAPI,
                                  std::make_shared<std::vector<ParameterInformation>>(
                                      std::vector<ParameterInformation>{{.name = "TestParameter"}}),
                                  pluginInterface.get()};
        auto tracedProcessInformation = createProcessInformation(tracedProcessDtb, tracedProcessUserDtb);

        ASSERT_NO_THROW(functionHook.hookFunction(testModuleBase, tracedProcessInformation));
    }

    TEST_F(FunctionHookTestFixture, hookFunction_validFunction_createsHook)
    {
        FunctionHook functionHook{std::string(testModuleName),
                                  std::string(testModuleFunctionName),
                                  extractor,
                                  introspectionAPI,
                                  std::make_shared<std::vector<ParameterInformation>>(
                                      std::vector<ParameterInformation>{{.name = "TestParameter"}}),
                                  pluginInterface.get()};
        EXPECT_CALL(*introspectionAPI, translateUserlandSymbolToVA).Times(1);
        EXPECT_CALL(*pluginInterface, createBreakpoint).Times(1);
        auto tracedProcessInformation = createProcessInformation(tracedProcessDtb, tracedProcessUserDtb);

        functionHook.hookFunction(testModuleBase, tracedProcessInformation);
    }

    TEST_F(FunctionHookTestFixture, hookCallBack_functionHookWithParameters_extractsParameters)
    {
        FunctionHook functionHook{std::string(testModuleName),
                                  std::string(testModuleFunctionName),
                                  extractor,
                                  introspectionAPI,
                                  std::make_shared<std::vector<ParameterInformation>>(
                                      std::vector<ParameterInformation>{{.name = "TestParameter"}}),
                                  pluginInterface.get()};
        auto tracedProcessInformation = createProcessInformation(tracedProcessDtb, tracedProcessUserDtb);

        functionHook.hookFunction(testModuleBase, tracedProcessInformation);
        EXPECT_CALL(*extractor, extractParameters).Times(1);

        functionHook.hookCallback(*interruptEvent);
    }
}
