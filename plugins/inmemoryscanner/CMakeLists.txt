cmake_minimum_required(VERSION 3.16)
project(inmemoryscanner)

set(INMEMORYSCANNER_VERSION "2.5.0" CACHE STRING "InMemory scanner version.")
set(INMEMORYSCANNER_BUILD_NUMBER "testbuild" CACHE STRING "InMemory scanner build number.")
option(INMEMORYSCANNER_TEST_COVERAGE "Build tests with coverage" OFF)
set(VMICORE_DIRECTORY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../vmicore" CACHE PATH "Path to directory root of VMICore project.")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
target_compile_definitions(inmemoryscanner-obj PRIVATE BUILD_VERSION="${INMEMORYSCANNER_BUILD_NUMBER}" PLUGIN_VERSION="${INMEMORYSCANNER_VERSION}")
target_compile_options(inmemoryscanner-obj PUBLIC -Wunused -Wunreachable-code -Wall -Wextra -Wpedantic)

include(CTest)
add_subdirectory(test)

if (INMEMORYSCANNER_TEST_COVERAGE)
    # Keep in mind that this will also propagate to all targets that use inmemoryscanner-obj (e.g. inmemoryscanner)
    target_compile_options(inmemoryscanner-obj PUBLIC --coverage)
    target_link_options(inmemoryscanner-obj PUBLIC --coverage)
endif ()
