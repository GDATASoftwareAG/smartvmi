cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(vmicore_public_headers INTERFACE)
target_sources(vmicore_public_headers INTERFACE
        vmicore/os/ActiveProcessInformation.h
        vmicore/os/IMemoryRegionExtractor.h
        vmicore/os/MemoryRegion.h
        vmicore/os/IPageProtection.h
        vmicore/plugins/IPluginConfig.h
        vmicore/plugins/PluginInit.h
        vmicore/plugins/PluginInterface.h
        vmicore/test/os/mock_MemoryRegionExtractor.h
        vmicore/test/os/mock_PageProtection.h
        vmicore/test/plugins/mock_PluginInterface.h)
target_include_directories(vmicore_public_headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
