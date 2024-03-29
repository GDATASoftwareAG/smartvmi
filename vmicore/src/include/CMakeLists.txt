cmake_minimum_required(VERSION 3.16)
project(vmicore)

add_library(vmicore-public-headers INTERFACE)
target_sources(vmicore-public-headers INTERFACE
        vmicore/io/ILogger.h
        vmicore/os/ActiveProcessInformation.h
        vmicore/os/IMemoryRegionExtractor.h
        vmicore/os/IPageProtection.h
        vmicore/os/MemoryRegion.h
        vmicore/os/OperatingSystem.h
        vmicore/os/PagingDefinitions.h
        vmicore/plugins/IPluginConfig.h
        vmicore/plugins/IPlugin.h
        vmicore/plugins/PluginInterface.h
        vmicore/vmi/BpResponse.h
        vmicore/callback.h
        vmicore/vmi/IBreakpoint.h
        vmicore/vmi/IIntrospectionAPI.h
        vmicore/vmi/IMemoryMapping.h
        vmicore/vmi/MappedRegion.h
        vmicore/vmi/events/IInterruptEvent.h
        vmicore/vmi/events/IRegisterReadable.h
        vmicore/filename.h
        vmicore/types.h)
target_include_directories(vmicore-public-headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(vmicore-public-headers INTERFACE cxx_std_20)
