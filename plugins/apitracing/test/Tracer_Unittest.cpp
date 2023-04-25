#include "../src/lib/Tracer.h"
#include "../src/lib/os/windows/Library.h"
#include "mock_Config.h"
#include "mock_FunctionDefinitions.h"
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vmicore_test/io/mock_Logger.h>
#include <vmicore_test/os/mock_MemoryRegionExtractor.h>
#include <vmicore_test/os/mock_PageProtection.h>
#include <vmicore_test/plugins/mock_PluginInterface.h>
#include <vmicore_test/vmi/mock_IntrospectionAPI.h>

using testing::_;
using testing::Contains;
using testing::NiceMock;
using testing::Not;
using testing::Return;
using VmiCore::addr_t;

namespace ApiTracing
{
    class TracerTestFixture : public testing::Test
    {
      protected:
        std::unique_ptr<NiceMock<VmiCore::Plugin::MockPluginInterface>> mockPluginInterface =
            std::make_unique<NiceMock<VmiCore::Plugin::MockPluginInterface>>();
        std::shared_ptr<NiceMock<VmiCore::MockIntrospectionAPI>> mockIntrospectionAPI =
            std::make_shared<NiceMock<VmiCore::MockIntrospectionAPI>>();
        std::shared_ptr<Tracer> tracer;

        static constexpr size_t size = 0x666;
        static constexpr pid_t testPid = 420;
        static constexpr addr_t processCr3 = 0x1337;

        static constexpr addr_t startAddress = 0x1234000;
        static constexpr addr_t startAddressShared = 0x1235000;
        static constexpr addr_t startAddressSharedNonDll = 0x1236000;

        static constexpr addr_t function1Address = 0x1234567;
        static constexpr addr_t ntdllFunctionAddress = 0x9876543210;

        std::shared_ptr<const VmiCore::ActiveProcessInformation> tracedProcessInformation;
        std::shared_ptr<const VmiCore::ActiveProcessInformation> untracedProcessInformation;

        std::string targetProcessName = "TraceMeNow.exe";
        std::string untracedProcessName = "DefinitelyNot.exe";

        std::string kernelDllName = "KernelBase.dll";
        std::string kernellDllFunctionName = "kernelDllFunction";
        std::string ntdllDllName = "ntdll.dll";
        std::string ntdllFunctionName = "ntdllFunction";
        std::string nonDllName = "KernelBase";

        VmiCore::MemoryRegion kernelDLLMemoryRegionDescriptor{
            startAddress, size, kernelDllName, std::make_unique<VmiCore::MockPageProtection>(), false, false, false};
        VmiCore::MemoryRegion ntDLLMemoryRegionDescriptor{startAddressShared,
                                                          size,
                                                          ntdllDllName,
                                                          std::make_unique<VmiCore::MockPageProtection>(),
                                                          false,
                                                          false,
                                                          false};
        VmiCore::MemoryRegion nonDLLMemoryRegionDescriptor{startAddressSharedNonDll,
                                                           size,
                                                           nonDllName,
                                                           std::make_unique<VmiCore::MockPageProtection>(),
                                                           false,
                                                           false,
                                                           false};

        std::pair<std::string, addr_t> expectedFirstModule =
            std::make_pair(kernelDLLMemoryRegionDescriptor.moduleName, kernelDLLMemoryRegionDescriptor.base);
        std::pair<std::string, addr_t> expectedSecondModule =
            std::make_pair(ntDLLMemoryRegionDescriptor.moduleName, ntDLLMemoryRegionDescriptor.base);
        std::pair<std::string, addr_t> unexpectedModule =
            std::make_pair(nonDLLMemoryRegionDescriptor.moduleName, nonDLLMemoryRegionDescriptor.base);

        std::vector<ProcessInformation> tracingInformation = {
            {false,
             targetProcessName,
             {{kernelDllName, {{kernellDllFunctionName}}}, {ntdllDllName, {{ntdllFunctionName}}}}}};

        void SetUp() override
        {
            ON_CALL(*mockPluginInterface, newNamedLogger(_))
                .WillByDefault([]() { return std::make_unique<NiceMock<VmiCore::MockLogger>>(); });

            tracer = std::make_shared<Tracer>(mockPluginInterface.get(),
                                              std::make_shared<MockConfig>(),
                                              std::make_shared<std::vector<ProcessInformation>>(tracingInformation),
                                              std::make_shared<MockFunctionDefinitions>(),
                                              std::make_shared<Windows::Library>());

            auto targetProcessMemoryRegionExtractor = std::make_unique<VmiCore::MockMemoryRegionExtractor>();
            ON_CALL(*targetProcessMemoryRegionExtractor, extractAllMemoryRegions())
                .WillByDefault(
                    [&kerneldllMemoryRegionDescriptor = kernelDLLMemoryRegionDescriptor,
                     &ntdllMemoryRegionDescriptor = ntDLLMemoryRegionDescriptor,
                     &nondllMemoryRegionDescriptor = nonDLLMemoryRegionDescriptor]()
                    {
                        auto memoryRegions = std::make_unique<std::list<VmiCore::MemoryRegion>>();
                        memoryRegions->push_back(std::move(kerneldllMemoryRegionDescriptor));
                        memoryRegions->push_back(std::move(ntdllMemoryRegionDescriptor));
                        memoryRegions->push_back(std::move(nondllMemoryRegionDescriptor));
                        return memoryRegions;
                    });
            tracedProcessInformation = std::make_shared<VmiCore::ActiveProcessInformation>(
                VmiCore::ActiveProcessInformation{0,
                                                  processCr3,
                                                  testPid,
                                                  0,
                                                  targetProcessName,
                                                  std::make_unique<std::string>(targetProcessName),
                                                  std::make_unique<std::string>(""),
                                                  std::move(targetProcessMemoryRegionExtractor),
                                                  true});
            untracedProcessInformation = std::make_shared<VmiCore::ActiveProcessInformation>(
                VmiCore::ActiveProcessInformation{0,
                                                  processCr3,
                                                  testPid,
                                                  0,
                                                  untracedProcessName,
                                                  std::make_unique<std::string>(untracedProcessName),
                                                  std::make_unique<std::string>(""),
                                                  std::make_unique<VmiCore::MockMemoryRegionExtractor>(),
                                                  true});

            ON_CALL(*mockPluginInterface, getIntrospectionAPI()).WillByDefault(Return(mockIntrospectionAPI));
            ON_CALL(
                *mockIntrospectionAPI,
                translateUserlandSymbolToVA(kernelDLLMemoryRegionDescriptor.base, processCr3, kernellDllFunctionName))
                .WillByDefault(Return(function1Address));
            ON_CALL(*mockIntrospectionAPI,
                    translateUserlandSymbolToVA(ntDLLMemoryRegionDescriptor.base, processCr3, ntdllFunctionName))
                .WillByDefault(Return(ntdllFunctionAddress));
        }
    };

    TEST_F(TracerTestFixture, initLoadedModules_validProcessInformation_correctModuleNamesStored)
    {
        ASSERT_NO_THROW(tracer->initLoadedModules(*tracedProcessInformation));

        EXPECT_THAT(tracer->getLoadedModules(), Contains(expectedFirstModule));
        EXPECT_THAT(tracer->getLoadedModules(), Contains(expectedSecondModule));
        EXPECT_THAT(tracer->getLoadedModules(), Not(Contains(unexpectedModule)));
    }

    TEST_F(TracerTestFixture, addHooks_tracedProcess_registerInterruptForAllHookTargets)
    {
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(function1Address, processCr3, _));
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(ntdllFunctionAddress, processCr3, _));
        ASSERT_NO_THROW(tracer->addHooks(*tracedProcessInformation));
    }

    TEST_F(TracerTestFixture, addHooks_untracedProcess_noInterruptRegistered)
    {
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(_, _, _)).Times(0);
        ASSERT_NO_THROW(tracer->addHooks(*untracedProcessInformation));
    }
}
