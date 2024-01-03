#include "TracedProcess.h"
#include "mock_FunctionDefinitions.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <os/windows/Library.h>
#include <string_view>
#include <vmicore_test/io/mock_Logger.h>
#include <vmicore_test/os/mock_MemoryRegionExtractor.h>
#include <vmicore_test/os/mock_PageProtection.h>
#include <vmicore_test/plugins/mock_PluginInterface.h>
#include <vmicore_test/vmi/mock_Breakpoint.h>
#include <vmicore_test/vmi/mock_IntrospectionAPI.h>

using testing::_;
using testing::NiceMock;
using testing::Ref;
using testing::Return;
using VmiCore::ActiveProcessInformation;
using VmiCore::addr_t;
using VmiCore::MemoryRegion;
using VmiCore::MockBreakpoint;
using VmiCore::MockIntrospectionAPI;
using VmiCore::Plugin::MockPluginInterface;

namespace ApiTracing
{
    namespace
    {
        constexpr std::string_view targetProcessName = "TraceMeNow.exe";
        constexpr pid_t tracedProcessPid = 420;
        constexpr addr_t tracedProcessDtb = 0x1337;
        constexpr addr_t tracedProcessUserDtb = 0x23456;

        constexpr size_t defaultDllSize = 0x666;
        constexpr addr_t kernelDllBase = 0x1234000;
        constexpr addr_t ntdllBase = 0x1235000;
        constexpr addr_t NonDllBase = 0x1236000;
        constexpr addr_t kernelDllFunctionAddress = 0x1234567;
        constexpr addr_t ntdllFunctionAddress = 0x9876543210;

        constexpr std::string_view kernelDllName = "KernelBase.dll";
        constexpr std::string_view kernellDllFunctionName = "kernelDllFunction";
        constexpr std::string_view ntdllDllName = "ntdll.dll";
        constexpr std::string_view ntdllFunctionName = "ntdllFunction";
        constexpr std::string_view nonDllName = "KernelBase";
    }

    MemoryRegion createMemoryRegionDescriptor(addr_t startAddr, size_t size, std::string_view name)
    {
        return MemoryRegion{
            startAddr, size, std::string(name), std::make_unique<VmiCore::MockPageProtection>(), false, false, false};
    }

    std::shared_ptr<const ActiveProcessInformation>
    createProcessInformationWithDefaultMemoryRegions(addr_t dtb, addr_t userDtb, pid_t pid, std::string_view name)
    {
        auto mockMemoryRegionExtractor = std::make_unique<VmiCore::MockMemoryRegionExtractor>();
        ON_CALL(*mockMemoryRegionExtractor, extractAllMemoryRegions())
            .WillByDefault(
                []()
                {
                    auto memoryRegions = std::make_unique<std::vector<MemoryRegion>>();
                    memoryRegions->push_back(
                        createMemoryRegionDescriptor(kernelDllBase, defaultDllSize, kernelDllName));
                    memoryRegions->push_back(createMemoryRegionDescriptor(ntdllBase, defaultDllSize, ntdllDllName));
                    memoryRegions->push_back(createMemoryRegionDescriptor(NonDllBase, defaultDllSize, nonDllName));
                    return memoryRegions;
                });

        return std::make_shared<const ActiveProcessInformation>(0,
                                                                dtb,
                                                                userDtb,
                                                                pid,
                                                                0,
                                                                std::string(name),
                                                                std::make_unique<std::string>(name),
                                                                std::make_unique<std::string>(""),
                                                                std::move(mockMemoryRegionExtractor),
                                                                true);
    }

    class TracedProcessTestFixture : public testing::Test
    {
      protected:
        std::unique_ptr<MockPluginInterface> mockPluginInterface = std::make_unique<MockPluginInterface>();
        std::shared_ptr<MockIntrospectionAPI> mockIntrospectionAPI = std::make_shared<MockIntrospectionAPI>();

        void SetUp() override
        {
            ON_CALL(*mockPluginInterface, newNamedLogger(_))
                .WillByDefault([]() { return std::make_unique<NiceMock<VmiCore::MockLogger>>(); });
            ON_CALL(*mockPluginInterface, getIntrospectionAPI()).WillByDefault(Return(mockIntrospectionAPI));
        }

        TracedProcess
        createTracedProcessWithDefaultDlls(std::shared_ptr<const ActiveProcessInformation> processInformation)
        {
            ON_CALL(*mockIntrospectionAPI,
                    translateUserlandSymbolToVA(kernelDllBase, _, std::string(kernellDllFunctionName)))
                .WillByDefault(Return(kernelDllFunctionAddress));
            ON_CALL(*mockIntrospectionAPI, translateUserlandSymbolToVA(ntdllBase, _, std::string(ntdllFunctionName)))
                .WillByDefault(Return(ntdllFunctionAddress));

            std::vector<ModuleInformation> defaultModuleTracingInformation = {
                {std::string(kernelDllName), {{std::string(kernellDllFunctionName)}}},
                {std::string(ntdllDllName), {{std::string(ntdllFunctionName)}}}};

            return {mockPluginInterface.get(),
                    std::make_shared<NiceMock<MockFunctionDefinitions>>(),
                    std::make_shared<Windows::Library>(),
                    std::move(processInformation),
                    {std::string(targetProcessName), true, defaultModuleTracingInformation}};
        }
    };

    TEST_F(TracedProcessTestFixture, constructor_defaultTracedProcess_correctBreakpointsInjected)
    {
        auto processInformation = createProcessInformationWithDefaultMemoryRegions(
            tracedProcessDtb, tracedProcessUserDtb, tracedProcessPid, targetProcessName);
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(kernelDllFunctionAddress, Ref(*processInformation), _))
            .WillOnce(Return(std::make_shared<NiceMock<MockBreakpoint>>()));
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(ntdllFunctionAddress, Ref(*processInformation), _))
            .WillOnce(Return(std::make_shared<NiceMock<MockBreakpoint>>()));

        EXPECT_NO_THROW(createTracedProcessWithDefaultDlls(processInformation));
    }

    TEST_F(TracedProcessTestFixture, destructor_defaultTracedProcess_breakpointsRemoved)
    {
        auto processInformation = createProcessInformationWithDefaultMemoryRegions(
            tracedProcessDtb, tracedProcessUserDtb, tracedProcessPid, targetProcessName);
        auto kernelDllFunctionBreakpoint = std::make_shared<MockBreakpoint>();
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(kernelDllFunctionAddress, Ref(*processInformation), _))
            .WillOnce(Return(kernelDllFunctionBreakpoint));
        auto ntdllFunctionBreakpoint = std::make_shared<MockBreakpoint>();
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(ntdllFunctionAddress, Ref(*processInformation), _))
            .WillOnce(Return(ntdllFunctionBreakpoint));

        EXPECT_CALL(*kernelDllFunctionBreakpoint, remove());
        EXPECT_CALL(*ntdllFunctionBreakpoint, remove());

        auto tracedProcess = createTracedProcessWithDefaultDlls(processInformation);
        tracedProcess.removeHooks();
    }
}
