#include "../src/lib/FunctionHook.h"
#include "mock_Extractor.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vmicore/vmi/IBreakpoint.h>
#include <vmicore_test/io/mock_Logger.h>
#include <vmicore_test/os/mock_MemoryRegionExtractor.h>
#include <vmicore_test/os/mock_PageProtection.h>
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
    class FunctionHookTestFixture : public testing::Test
    {
      protected:
        std::unique_ptr<MockPluginInterface> pluginInterface = std::make_unique<NiceMock<MockPluginInterface>>();
        std::shared_ptr<MockIntrospectionAPI> introspectionAPI = std::make_shared<NiceMock<MockIntrospectionAPI>>();
        std::shared_ptr<MockExtractor> extractor = std::make_shared<NiceMock<MockExtractor>>();
        std::shared_ptr<MockInterruptEvent> interruptEvent = std::make_shared<NiceMock<MockInterruptEvent>>();
        std::shared_ptr<ActiveProcessInformation> tracedProcessInformation;

        const uint64_t kernelspaceLowerBoundary = 0xFFFF800000000000;
        const addr_t tracedProcessKernelDtb = 0x1338;
        const addr_t tracedProcessUserDtb = 0x1337;
        const addr_t kernelDllBase = 0x1234000 + kernelspaceLowerBoundary;
        const addr_t kernelDllFunctionAddress = 0x1234567;
        const std::string_view kernelDllName = "KernelBase.dll";
        const std::string_view kernelDllFunctionName = "kernelDllFunction";

        void SetUp() override
        {
            ON_CALL(*interruptEvent, getCr3()).WillByDefault(Return(tracedProcessKernelDtb));
            ON_CALL(*pluginInterface, newNamedLogger(_))
                .WillByDefault([]() { return std::make_unique<NiceMock<VmiCore::MockLogger>>(); });
            ON_CALL(
                *introspectionAPI,
                translateUserlandSymbolToVA(kernelDllBase, tracedProcessKernelDtb, std::string(kernelDllFunctionName)))
                .WillByDefault(Return(kernelDllFunctionAddress));

            tracedProcessInformation =
                std::make_shared<ActiveProcessInformation>(0,
                                                           tracedProcessKernelDtb,
                                                           tracedProcessUserDtb,
                                                           0,
                                                           0,
                                                           std::string(""),
                                                           std::make_unique<std::string>(""),
                                                           std::make_unique<std::string>(""),
                                                           std::make_unique<VmiCore::MockMemoryRegionExtractor>(),
                                                           true);
        }
    };

    TEST_F(FunctionHookTestFixture, hookFunction_validFunction_noThrow)
    {
        FunctionHook functionHook{std::string(kernelDllName),
                                  std::string(kernelDllFunctionName),
                                  extractor,
                                  introspectionAPI,
                                  std::make_shared<std::vector<ParameterInformation>>(
                                      std::vector<ParameterInformation>{{.name = "TestParameter"}}),
                                  pluginInterface.get()};

        ASSERT_NO_THROW(functionHook.hookFunction(kernelDllBase, tracedProcessInformation));
    }

    TEST_F(FunctionHookTestFixture, hookFunction_validFunction_createsHook)
    {
        FunctionHook functionHook{std::string(kernelDllName),
                                  std::string(kernelDllFunctionName),
                                  extractor,
                                  introspectionAPI,
                                  std::make_shared<std::vector<ParameterInformation>>(
                                      std::vector<ParameterInformation>{{.name = "TestParameter"}}),
                                  pluginInterface.get()};
        EXPECT_CALL(*introspectionAPI, translateUserlandSymbolToVA).Times(1);
        EXPECT_CALL(*pluginInterface, createBreakpoint).Times(1);

        functionHook.hookFunction(kernelDllBase, tracedProcessInformation);
    }

    TEST_F(FunctionHookTestFixture, hookCallBack_functionHookWithParameters_extractsParameters)
    {
        FunctionHook functionHook{std::string(kernelDllName),
                                  std::string(kernelDllFunctionName),
                                  extractor,
                                  introspectionAPI,
                                  std::make_shared<std::vector<ParameterInformation>>(
                                      std::vector<ParameterInformation>{{.name = "TestParameter"}}),
                                  pluginInterface.get()};

        functionHook.hookFunction(kernelDllBase, tracedProcessInformation);
        EXPECT_CALL(*extractor, extractParameters).Times(1);

        functionHook.hookCallback(*interruptEvent);
    }
}
