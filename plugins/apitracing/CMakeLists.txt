cmake_minimum_required(VERSION 3.16)
project(apitracing)

set(APITRACING_VERSION "0.1" CACHE STRING "Apitracing version.")
set(APITRACING_BUILD_NUMBER "testbuild" CACHE STRING "Apitracing Build number.")
option(APITRACING_TEST_COVERAGE "Build tests with coverage" OFF)
set(VMICORE_DIRECTORY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../vmicore" CACHE PATH "Path to directory root of VMICore project.")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
target_compile_definitions(apitracing-obj PRIVATE BUILD_VERSION="${APITRACING_BUILD_NUMBER}" PLUGIN_VERSION="${APITRACING_VERSION}")
target_compile_options(apitracing-obj PUBLIC -Wunused -Wunreachable-code -Wall -Wextra -Wpedantic)

include(CTest)
add_subdirectory(test)

if (APITRACING_TEST_COVERAGE)
    # Keep in mind that this will also propagate to all targets that use apitracing-obj (e.g. apitracing)
    target_compile_options(apitracing-obj PUBLIC --coverage)
    target_link_options(apitracing-obj PUBLIC --coverage)
endif ()
