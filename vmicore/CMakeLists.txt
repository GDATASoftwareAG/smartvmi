cmake_minimum_required(VERSION 3.16)
project(vmicore)

set(VMICORE_PROGRAM_VERSION "0.0.0" CACHE STRING "Program version.")
set(VMICORE_PROGRAM_BUILD_NUMBER "testbuild" CACHE STRING "Build number.")
option(VMICORE_TRACE_MODE "Include extra tracing output" OFF)
option(VMICORE_TEST_COVERAGE "Build tests with coverage" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
target_compile_definitions(vmicore PRIVATE PROGRAM_VERSION="${VMICORE_PROGRAM_VERSION}" BUILD_VERSION="${VMICORE_PROGRAM_BUILD_NUMBER}")
target_compile_options(vmicore-lib PUBLIC -Wall -Wunused -Wunreachable-code -Wextra)
set_target_properties(vmicore PROPERTIES CXX_INCLUDE_WHAT_YOU_USE "include-what-you-use;-Xiwyu;--no_fwd_decls;-Xiwyu;--cxx17ns;-Xiwyu;--mapping-file=mappings.imp")

if (VMICORE_TRACE_MODE)
    target_compile_definitions(vmicore-lib PRIVATE TRACE_MODE)
endif ()

include(CTest)
add_subdirectory(test)

if (VMICORE_TEST_COVERAGE)
    # Keep in mind that this will also propagate to all targets that use vmicore-lib (e.g. vmicore)
    target_compile_options(vmicore-lib PUBLIC --coverage)
    target_link_options(vmicore-lib PUBLIC --coverage)
endif ()
