cmake_minimum_required(VERSION 3.16)
project(stringfinder)

set(STRINGFINDER_VERSION "0.1" CACHE STRING "StringFinder plugin version.")
set(STRINGFINDER_BUILD_NUMBER "testbuild" CACHE STRING "StringFinder plugin Build number.")
set(VMICORE_DIRECTORY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../vmicore" CACHE PATH "Path to directory root of VMICore project.")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
target_compile_definitions(stringfinder-obj PRIVATE BUILD_VERSION="${STRINGFINDER_BUILD_NUMBER}" PLUGIN_VERSION="${STRINGFINDER_VERSION}")
target_compile_options(stringfinder-obj PUBLIC -Wunused -Wunreachable-code -Wall -Wextra -Wpedantic)
