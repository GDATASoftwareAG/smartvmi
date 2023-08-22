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
using testing::Return;
using VmiCore::ActiveProcessInformation;
using VmiCore::addr_t;
using VmiCore::MemoryRegion;
using VmiCore::MockBreakpoint;
using VmiCore::MockIntrospectionAPI;
using VmiCore::Plugin::MockPluginInterface;

namespace ApiTracing
{
    class TracedProcessTestFixture : public testing::Test
    {
      protected:
        std::unique_ptr<MockPluginInterface> mockPluginInterface = std::make_unique<MockPluginInterface>();
        std::shared_ptr<MockIntrospectionAPI> mockIntrospectionAPI = std::make_shared<MockIntrospectionAPI>();
        std::shared_ptr<const ActiveProcessInformation> tracedProcessInformation;

        const uint64_t kernelspaceLowerBoundary = 0xFFFF800000000000;

        const std::string_view targetProcessName = "TraceMeNow.exe";
        const pid_t tracedProcessPid = 420;
        const addr_t tracedProcessKernelDtb = 0x1337;
        const addr_t tracedProcessUserDtb = 0x1337;

        const size_t size = 0x666;
        const addr_t kernelDllBase = 0x1234000 + kernelspaceLowerBoundary;
        const addr_t ntdllBase = 0x1235000;
        const addr_t NonDllBase = 0x1236000;
        const addr_t kernelDllFunctionAddress = 0x1234567;
        const addr_t ntdllFunctionAddress = 0x9876543210;

        const std::string_view kernelDllName = "KernelBase.dll";
        const std::string_view kernellDllFunctionName = "kernelDllFunction";
        const std::string_view ntdllDllName = "ntdll.dll";
        const std::string_view ntdllFunctionName = "ntdllFunction";
        const std::string_view nonDllName = "KernelBase";

        void SetUp() override
        {
            ON_CALL(*mockPluginInterface, newNamedLogger(_))
                .WillByDefault([]() { return std::make_unique<NiceMock<VmiCore::MockLogger>>(); });
            ON_CALL(*mockPluginInterface, getIntrospectionAPI()).WillByDefault(Return(mockIntrospectionAPI));
        }

        static MemoryRegion createMemoryRegionDescriptor(addr_t startAddr, size_t size, std::string_view name)
        {
            return MemoryRegion{startAddr,
                                size,
                                std::string(name),
                                std::make_unique<VmiCore::MockPageProtection>(),
                                false,
                                false,
                                false};
        }

        void setupProcessInformation()
        {
            auto mockMemoryRegionExtractor = std::make_unique<VmiCore::MockMemoryRegionExtractor>();
            ON_CALL(*mockMemoryRegionExtractor, extractAllMemoryRegions())
                .WillByDefault(
                    [this]()
                    {
                        auto memoryRegions = std::make_unique<std::list<MemoryRegion>>();
                        memoryRegions->push_back(createMemoryRegionDescriptor(kernelDllBase, size, kernelDllName));
                        memoryRegions->push_back(createMemoryRegionDescriptor(ntdllBase, size, ntdllDllName));
                        memoryRegions->push_back(createMemoryRegionDescriptor(NonDllBase, size, nonDllName));
                        return memoryRegions;
                    });

            tracedProcessInformation =
                std::make_shared<const ActiveProcessInformation>(0,
                                                                 tracedProcessKernelDtb,
                                                                 tracedProcessUserDtb,
                                                                 tracedProcessPid,
                                                                 0,
                                                                 std::string(targetProcessName),
                                                                 std::make_unique<std::string>(targetProcessName),
                                                                 std::make_unique<std::string>(""),
                                                                 std::move(mockMemoryRegionExtractor),
                                                                 true);
        };

        TracedProcess createDefaultTracedProcess()
        {
            std::vector<ModuleInformation> defaultModuleTracingInformation = {
                {std::string(kernelDllName), {{std::string(kernellDllFunctionName)}}},
                {std::string(ntdllDllName), {{std::string(ntdllFunctionName)}}}};

            ON_CALL(*mockIntrospectionAPI,
                    translateUserlandSymbolToVA(kernelDllBase, _, std::string(kernellDllFunctionName)))
                .WillByDefault(Return(kernelDllFunctionAddress));
            ON_CALL(*mockIntrospectionAPI, translateUserlandSymbolToVA(ntdllBase, _, std::string(ntdllFunctionName)))
                .WillByDefault(Return(ntdllFunctionAddress));

            return {mockPluginInterface.get(),
                    std::make_shared<NiceMock<MockFunctionDefinitions>>(),
                    std::make_shared<Windows::Library>(),
                    tracedProcessInformation,
                    {std::string(targetProcessName), true, defaultModuleTracingInformation}};
        }
    };

    TEST_F(TracedProcessTestFixture, constructor_defaultTracedProcess_correctBreakpointsInjected)
    {
        setupProcessInformation();

        EXPECT_CALL(*mockPluginInterface, createBreakpoint(kernelDllFunctionAddress, tracedProcessInformation, _))
            .WillOnce(Return(std::make_shared<NiceMock<MockBreakpoint>>()));
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(ntdllFunctionAddress, tracedProcessInformation, _))
            .WillOnce(Return(std::make_shared<NiceMock<MockBreakpoint>>()));

        EXPECT_NO_THROW(createDefaultTracedProcess());
    }

    TEST_F(TracedProcessTestFixture, destructor_defaultTracedProcess_breakpointsRemoved)
    {
        setupProcessInformation();

        auto kernelDllFunctionBreakpoint = std::make_shared<MockBreakpoint>();
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(kernelDllFunctionAddress, tracedProcessInformation, _))
            .WillOnce(Return(kernelDllFunctionBreakpoint));
        auto ntdllFunctionBreakpoint = std::make_shared<MockBreakpoint>();
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(ntdllFunctionAddress, tracedProcessInformation, _))
            .WillOnce(Return(ntdllFunctionBreakpoint));

        EXPECT_CALL(*kernelDllFunctionBreakpoint, remove());
        EXPECT_CALL(*ntdllFunctionBreakpoint, remove());

        auto tracedProcess = createDefaultTracedProcess();
        tracedProcess.removeHooks();
    }
}
