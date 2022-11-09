cmake_minimum_required(VERSION 3.16)
project(vmicore_test)

if (NOT TARGET vmicore-public-headers)
    message(SEND_ERROR "VmiCore public headers target missing.")
endif ()

if (NOT TARGET gmock)
    message(SEND_ERROR "Google mock target missing.")
endif ()

add_library(vmicore-public-test-headers INTERFACE)
target_sources(vmicore-public-test-headers INTERFACE
        vmicore_test/os/mock_MemoryRegionExtractor.h
        vmicore_test/os/mock_PageProtection.h
        vmicore_test/plugins/mock_PluginInterface.h
        vmicore_test/vmi/mock_Breakpoint.h
        vmicore_test/vmi/mock_InterruptEvent.h
        vmicore_test/vmi/mock_IntrospectionAPI.h)
target_include_directories(vmicore-public-test-headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(vmicore-public-test-headers INTERFACE cxx_std_20)
target_link_libraries(vmicore-public-test-headers INTERFACE vmicore-public-headers gmock)
