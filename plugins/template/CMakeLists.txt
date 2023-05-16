cmake_minimum_required(VERSION 3.16)
project(template)

set(TEMPLATE_VERSION "0.1" CACHE STRING "Template version.")
set(TEMPLATE_BUILD_NUMBER "testbuild" CACHE STRING "Template Build number.")
option(TEMPLATE_TEST_COVERAGE "Build tests with coverage" OFF)
set(VMICORE_DIRECTORY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../vmicore" CACHE PATH "Path to directory root of VMICore project.")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
target_compile_definitions(template-obj PRIVATE BUILD_VERSION="${TEMPLATE_BUILD_NUMBER}" PLUGIN_VERSION="${TEMPLATE_VERSION}")
target_compile_options(template-obj PUBLIC -Wunused -Wunreachable-code -Wall -Wextra -Wpedantic)

include(CTest)
add_subdirectory(test)

if (TEMPLATE_TEST_COVERAGE)
    # Keep in mind that this will also propagate to all targets that use template-obj (e.g. template)
    target_compile_options(template-obj PUBLIC --coverage)
    target_link_options(template-obj PUBLIC --coverage)
endif ()
