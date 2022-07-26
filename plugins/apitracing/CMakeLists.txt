cmake_minimum_required(VERSION 3.16)
project(apitracing)

set(PROGRAM_VERSION "0.1" CACHE STRING "Program version.")
set(PROGRAM_BUILD_NUMBER "testbuild" CACHE STRING "Build number.")
set(VMICORE_DIRECTORY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../vmicore" CACHE PATH "Path to directory root of VMICore project.")

# Dependencies

include(FindPkgConfig)

include(FetchContent)

# Setup bundled google test framework

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.11.0
)

# Setup fmt library as it is not available yet in gnu stdlib
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 8.1.1
)
FetchContent_MakeAvailable(fmt)

set(googletest_BUILD_GMOCK ON)
set(googletest_BUILD_GTEST ON)
set(googletest_BUILD_SHARED_LIBS ON)
set(googletest_INSTALL_GTEST OFF)
set(googletest_INSTALL_GMOCK OFF)
FetchContent_MakeAvailable(googletest)

# Configure include directory for required VMICore plugin header files

include_directories("${VMICORE_DIRECTORY_ROOT}/src/plugins")

# Variable section

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(compile_flags -Wunused -Wunreachable-code -Wall -Wextra -Wpedantic)

add_definitions(-DBUILD_VERSION="${PROGRAM_BUILD_NUMBER}" -DPLUGIN_NAME="${PROJECT_NAME}" -DPLUGIN_VERSION="${PROGRAM_VERSION}")

# End variable section

set(source_files
        src/Config.cpp
        src/ApiTracing.cpp)

set(test_files)

set(libraries fmt-header-only)

add_library(apitracing MODULE ${source_files})

target_compile_options(apitracing PRIVATE ${compile_flags})
target_link_libraries(apitracing ${libraries})

add_executable(apitracing-test ${source_files} ${test_files})

target_compile_options(apitracing-test PRIVATE ${compile_flags} -g -O0 --coverage)
target_link_options(apitracing-test PRIVATE --coverage)
target_link_libraries(apitracing-test ${libraries} pthread gtest gmock gmock_main)

# Setup test discovery

include(GoogleTest)

enable_testing()
gtest_discover_tests(apitracing-test)
