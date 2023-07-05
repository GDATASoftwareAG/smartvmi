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
        static constexpr pid_t tracedProcessPid = 420;
        static constexpr pid_t untracedProcessPid = 4711;
        static constexpr pid_t tracedChildPid = 42;
        static constexpr addr_t tracedProcessCr3 = 0x1337;
        static constexpr addr_t untracedProcessCr3 = 0x815;
        static constexpr addr_t tracedChildCr3 = 0x691;

        static constexpr addr_t startAddress = 0x1234000;
        static constexpr addr_t startAddressShared = 0x1235000;
        static constexpr addr_t startAddressSharedNonDll = 0x1236000;

        static constexpr addr_t function1Address = 0x1234567;
        static constexpr addr_t ntdllFunctionAddress = 0x9876543210;

        std::shared_ptr<const VmiCore::ActiveProcessInformation> tracedProcessInformation;
        std::shared_ptr<const VmiCore::ActiveProcessInformation> untracedProcessInformation;
        std::shared_ptr<const VmiCore::ActiveProcessInformation> untracedProcessWithTracedParentInformation;
        std::shared_ptr<const VmiCore::ActiveProcessInformation> tracedProcessWithConflictingParentConfig;
        std::shared_ptr<const VmiCore::ActiveProcessInformation> dontTraceChildProcess;
        std::shared_ptr<const VmiCore::ActiveProcessInformation> childOfDontTraceChildProcess;

        std::string targetProcessName = "TraceMeNow.exe";
        std::string untracedProcessName = "DefinitelyNot.exe";
        std::string emptyConfigProcessName = "EmptyConfigProcess.exe";
        std::string dontTraceChildProcessName = "DontTraceMyChildEverAgain.exe";

        std::string kernelDllName = "KernelBase.dll";
        std::string kernellDllFunctionName = "kernelDllFunction";
        std::string ntdllDllName = "ntdll.dll";
        std::string ntdllFunctionName = "ntdllFunction";
        std::string nonDllName = "KernelBase";

        VmiCore::MemoryRegion kernelDLLMemoryRegionDescriptor =
            createMemoryRegionDescriptor(startAddress, size, kernelDllName);
        VmiCore::MemoryRegion ntDLLMemoryRegionDescriptor =
            createMemoryRegionDescriptor(startAddressShared, size, ntdllDllName);
        VmiCore::MemoryRegion nonDLLMemoryRegionDescriptor =
            createMemoryRegionDescriptor(startAddressSharedNonDll, size, nonDllName);

        std::pair<std::string, addr_t> expectedFirstModule =
            std::make_pair(kernelDLLMemoryRegionDescriptor.moduleName, kernelDLLMemoryRegionDescriptor.base);
        std::pair<std::string, addr_t> expectedSecondModule =
            std::make_pair(ntDLLMemoryRegionDescriptor.moduleName, ntDLLMemoryRegionDescriptor.base);
        std::pair<std::string, addr_t> unexpectedModule =
            std::make_pair(nonDLLMemoryRegionDescriptor.moduleName, nonDLLMemoryRegionDescriptor.base);

        std::vector<ModuleInformation> moduleTracingInformation = std::vector<ModuleInformation>{
            {kernelDllName, {{kernellDllFunctionName}}}, {ntdllDllName, {{ntdllFunctionName}}}};

        void SetUp() override
        {
            ON_CALL(*mockPluginInterface, newNamedLogger(_))
                .WillByDefault([]() { return std::make_unique<NiceMock<VmiCore::MockLogger>>(); });

            std::unique_ptr<MockConfig> mockTracingTargetsParser = std::make_unique<MockConfig>();
            setupTracingTargetsParser(*mockTracingTargetsParser,
                                      {{targetProcessName, true, moduleTracingInformation},
                                       {emptyConfigProcessName, false, {}},
                                       {dontTraceChildProcessName, false, moduleTracingInformation}});

            tracer = std::make_shared<Tracer>(mockPluginInterface.get(),
                                              std::move(mockTracingTargetsParser),
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
            auto tracedChildMemoryRegionExtractor = std::make_unique<VmiCore::MockMemoryRegionExtractor>();
            ON_CALL(*tracedChildMemoryRegionExtractor, extractAllMemoryRegions())
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

            tracedProcessInformation =
                createActiveProcessInformation(targetProcessName, tracedProcessCr3, tracedProcessPid, 0);
            untracedProcessInformation = std::make_shared<VmiCore::ActiveProcessInformation>(
                VmiCore::ActiveProcessInformation{0,
                                                  untracedProcessCr3,
                                                  untracedProcessPid,
                                                  0,
                                                  untracedProcessName,
                                                  std::make_unique<std::string>(untracedProcessName),
                                                  std::make_unique<std::string>(""),
                                                  std::make_unique<VmiCore::MockMemoryRegionExtractor>(),
                                                  true});
            untracedProcessWithTracedParentInformation =
                createActiveProcessInformation(untracedProcessName, tracedChildCr3, tracedChildPid, tracedProcessPid);
            tracedProcessWithConflictingParentConfig = createActiveProcessInformation(
                emptyConfigProcessName, tracedChildCr3, tracedChildPid, tracedProcessPid);
            dontTraceChildProcess =
                createActiveProcessInformation(dontTraceChildProcessName, tracedProcessCr3, tracedProcessPid, 0);
            childOfDontTraceChildProcess =
                createActiveProcessInformation(targetProcessName, tracedChildCr3, tracedChildPid, tracedProcessPid);

            ON_CALL(*mockPluginInterface, getIntrospectionAPI()).WillByDefault(Return(mockIntrospectionAPI));
            ON_CALL(*mockIntrospectionAPI,
                    translateUserlandSymbolToVA(kernelDLLMemoryRegionDescriptor.base, _, kernellDllFunctionName))
                .WillByDefault(Return(function1Address));
            ON_CALL(*mockIntrospectionAPI,
                    translateUserlandSymbolToVA(ntDLLMemoryRegionDescriptor.base, _, ntdllFunctionName))
                .WillByDefault(Return(ntdllFunctionAddress));
        }

        std::shared_ptr<VmiCore::ActiveProcessInformation>
        createActiveProcessInformation(std::string processName, uint64_t processCr3, pid_t processPid, pid_t parentPid)
        {
            auto mockMemoryRegionExtractor = std::make_unique<VmiCore::MockMemoryRegionExtractor>();
            ON_CALL(*mockMemoryRegionExtractor, extractAllMemoryRegions())
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
            return std::make_shared<VmiCore::ActiveProcessInformation>(
                VmiCore::ActiveProcessInformation{0,
                                                  processCr3,
                                                  processPid,
                                                  parentPid,
                                                  processName,
                                                  std::make_unique<std::string>(processName),
                                                  std::make_unique<std::string>(""),
                                                  std::move(mockMemoryRegionExtractor),
                                                  true});
        }

        static void setupTracingTargetsParser(const MockConfig& tracingTargetsParser,
                                              const std::vector<TracingProfile>& tracingProfiles)
        {
            for (const auto& profile : tracingProfiles)
            {
                ON_CALL(tracingTargetsParser, getTracingProfile(profile.name)).WillByDefault(Return(profile));
            }
        }

        static VmiCore::MemoryRegion
        createMemoryRegionDescriptor(addr_t startAddr, size_t regionSize, const std::string& name)
        {
            return {startAddr, regionSize, name, std::make_unique<VmiCore::MockPageProtection>(), false, false, false};
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
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(function1Address, tracedProcessCr3, _));
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(ntdllFunctionAddress, tracedProcessCr3, _));
        ASSERT_NO_THROW(tracer->addHooks(tracedProcessInformation));
    }

    TEST_F(TracerTestFixture, addHooks_untracedProcess_noInterruptRegistered)
    {
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(_, _, _)).Times(0);
        ASSERT_NO_THROW(tracer->addHooks(untracedProcessInformation));
    }

    TEST_F(TracerTestFixture, addHooks_noProcessProfileTracedParent_createBreakpointCalled)
    {
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(function1Address, tracedProcessCr3, _));
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(ntdllFunctionAddress, tracedProcessCr3, _));
        ASSERT_NO_THROW(tracer->addHooks(tracedProcessInformation));
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(function1Address, tracedChildCr3, _));
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(ntdllFunctionAddress, tracedChildCr3, _));

        ASSERT_NO_THROW(tracer->addHooks(untracedProcessWithTracedParentInformation));
    }

    TEST_F(TracerTestFixture, addHooks_childWithOwnConfig_parentProcessConfigUsed)
    {
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(function1Address, tracedProcessCr3, _));
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(ntdllFunctionAddress, tracedProcessCr3, _));
        ASSERT_NO_THROW(tracer->addHooks(tracedProcessInformation));
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(function1Address, tracedChildCr3, _));
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(ntdllFunctionAddress, tracedChildCr3, _));

        ASSERT_NO_THROW(tracer->addHooks(tracedProcessWithConflictingParentConfig));
    }

    TEST_F(TracerTestFixture, addHooks_processWithoutTraceChildren_onlyParentTraced)
    {
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(function1Address, tracedProcessCr3, _));
        EXPECT_CALL(*mockPluginInterface, createBreakpoint(ntdllFunctionAddress, tracedProcessCr3, _));
        ASSERT_NO_THROW(tracer->addHooks(dontTraceChildProcess));

        ASSERT_NO_THROW(tracer->addHooks(childOfDontTraceChildProcess));
    }
}
