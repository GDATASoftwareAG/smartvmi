cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(vmicore_mocks INTERFACE)
target_sources(vmicore_mocks INTERFACE
        include/vmicore/os/mock_MemoryRegionExtractor.h
        include/vmicore/plugins/mock_PluginInterface.h)
target_include_directories(vmicore_mocks INTERFACE include/)
