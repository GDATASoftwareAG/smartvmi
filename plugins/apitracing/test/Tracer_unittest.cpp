#include "../src/Tracer.h"
#include "mock_Config.h"
#include "mock_PluginInterface.h"
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::Unused;
using testing::NiceMock;
using testing::Contains;
using testing::Not;

class TracerTestFixture : public testing::Test
{
  protected:
    std::unique_ptr<Plugin::MockPluginInterface> pluginInterface = std::make_unique<NiceMock<Plugin::MockPluginInterface>>();
    std::shared_ptr<MockConfig> configuration = std::make_shared<MockConfig>();
    std::optional<Tracer> tracer;

    size_t size = 0x666;
    ProtectionValues protectionFlags = ProtectionValues::PAGE_GUARD_PAGE_EXECUTE_READWRITE;
    pid_t testPid = 420;

    Plugin::virtual_address_t startAddress = 0x1234000;
    std::string dllPath = "\\\\Windows\\\\System32\\\\KernelBase.dll";
    Plugin::MemoryRegion memoryRegionDescriptor{startAddress, size, dllPath, protectionFlags, false, false, false};
    Plugin::virtual_address_t startAddressShared = 0x1235000;
    std::string dllPathShared = "\\\\Windows\\\\System32\\\\ntdll.dll";
    Plugin::MemoryRegion memoryRegionDescriptorForSharedMemory{
        startAddressShared, size, dllPathShared, protectionFlags, true, false, true};
    Plugin::virtual_address_t startAddressSharedNonDll = 0x1236000;
    std::string nonDllPathShared = "\\\\Windows\\\\System32\\\\KernelBase";
    Plugin::MemoryRegion nonDllMemoryRegionDescriptorForSharedMemory{
        startAddressSharedNonDll, size, nonDllPathShared, protectionFlags, true, false, true};
    std::vector<Plugin::MemoryRegion> memoryRegions = {
        memoryRegionDescriptor, memoryRegionDescriptorForSharedMemory, nonDllMemoryRegionDescriptorForSharedMemory};

    std::shared_ptr<std::string> tracedProcess = std::make_shared<std::string>("traceMeNow");

    std::string kernelDllName = "KernelBase.dll";
    std::string ntdllDllName = "ntdll.dll";
    std::string nonDllName = "KernelBase";
    std::pair<std::string, Plugin::virtual_address_t> expectedFirstModule = std::make_pair(kernelDllName, startAddress);
    std::pair<std::string, Plugin::virtual_address_t> expectedSecondModule =
        std::make_pair(ntdllDllName, startAddressShared);
    std::pair<std::string, Plugin::virtual_address_t> unExpectedModule =
        std::make_pair(nonDllName, startAddressSharedNonDll);

    void SetUp() override
    {
        ON_CALL(*pluginInterface, getProcessMemoryRegions(testPid))
            .WillByDefault([&memoryRegions = memoryRegions](Unused)
                           { return std::make_unique<std::vector<Plugin::MemoryRegion>>(memoryRegions); });

        tracer.emplace(pluginInterface.get(), configuration);
    }
};

MATCHER_P(IsEqualModule, expectedModule, "")
{
    bool isEqual = false;

    auto filenameMatch = expectedModule.first == arg.first;
    auto baseAddressMatch = expectedModule.second == arg.second;

    if (filenameMatch && baseAddressMatch)
    {
        isEqual = true;
    }

    return isEqual;
}

TEST_F(TracerTestFixture, initLoadedModules_validPid_correctModuleNamesStored)
{
    ASSERT_NO_THROW(tracer->initLoadedModules(testPid));

    EXPECT_THAT(tracer->getLoadedModules(), Contains(IsEqualModule(expectedFirstModule)));
    EXPECT_THAT(tracer->getLoadedModules(), Contains(IsEqualModule(expectedSecondModule)));
    EXPECT_THAT(tracer->getLoadedModules(), Not(Contains(IsEqualModule(unExpectedModule))));
}

TEST_F(TracerTestFixture, addHooks_tracedProcess_registerInterruptForAllHookTargets)
{
//    EXPECT_CALL(*pluginInterface,
//               requestFunctionCallHook()); //TODO Create function with fitting parameters (process-cr3, address, callback)

    ASSERT_NO_THROW(tracer->addHooks(testPid, tracedProcess));
}
