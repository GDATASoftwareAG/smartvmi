#include <vmicore_test/io/mock_Logger.h>
#include <vmicore_test/os/mock_MemoryRegionExtractor.h>
#include <vmicore_test/os/mock_PageProtection.h>
#include <vmicore_test/plugins/mock_PluginConfig.h>
#include <vmicore_test/plugins/mock_PluginInterface.h>
#include <vmicore_test/vmi/mock_Breakpoint.h>
#include <vmicore_test/vmi/mock_InterruptEvent.h>
#include <vmicore_test/vmi/mock_IntrospectionAPI.h>

[[maybe_unused]] void detectPureVirtualFunctions()
{
    VmiCore::MockLogger mockLogger;
    VmiCore::MockMemoryRegionExtractor mockMemoryRegionExtractor;
    VmiCore::MockPageProtection mockPageProtection;
    VmiCore::Plugin::MockPluginConfig mockPluginConfig;
    VmiCore::Plugin::MockPluginInterface mockPluginInterface;
    VmiCore::MockBreakpoint mockBreakpoint;
    VmiCore::MockInterruptEvent mockInterruptEvent;
    VmiCore::MockIntrospectionAPI mockIntrospectionApi;
}
