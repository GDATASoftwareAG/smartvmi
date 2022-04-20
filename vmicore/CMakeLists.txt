cmake_minimum_required(VERSION 3.16)
project(vmicore)


# Options

set(PROGRAM_VERSION "0.0.0" CACHE STRING "Program version.")
set(PROGRAM_BUILD_NUMBER "testbuild" CACHE STRING "Build number.")
option(TRACE_MODE "Include extra tracing output" OFF)
option(VMI_SANITIZERS "Build with sanitizers." OFF)

# Variable section

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(core_compile_flags -m64 -Wall)
set(extra_compile_flags -Wunused -Wunreachable-code -Wextra -fPIE -Wpedantic)
set(libraries vmi_shared dl yaml-cpp)

# Toolchain checks

include(CheckPIESupported)
check_pie_supported()


find_library(libubsan_static libubsan.a HINTS /usr/lib/gcc/*/*/)
find_library(libasan_static libasan.a HINTS /usr/lib/gcc/*/*/)
if (VMI_SANITIZERS)
    if (libubsan_static AND libasan_static)
        set(extra_compile_flags ${extra_compile_flags} -fsanitize=undefined -fno-omit-frame-pointer -fsanitize=address)
        set(extra_libraries pthread libubsan.a libasan.a)
    else ()
        message("Unable to find sanitizer libraries: ${libubsan_static} ${libasan_static}")
    endif ()
endif ()

add_definitions(-DPROGRAM_VERSION="${PROGRAM_VERSION}" -DBUILD_VERSION="${PROGRAM_BUILD_NUMBER}")
if (TRACE_MODE)
    add_definitions(-DTRACE_MODE)
endif ()

include(FetchContent)

# Setup bundled google test framework

set(BUILD_GMOCK ON CACHE BOOL "Build google mock.")
set(BUILD_GTEST ON CACHE BOOL "Build google test.")
set(BUILD_SHARED_LIBS ON CACHE BOOL "Create shared libraries.")
set(INSTALL_GTEST OFF CACHE BOOL "Do not create installing functionality.")
set(INSTALL_GMOCK OFF CACHE BOOL "Do not create installing functionality.")
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.11.0
)
FetchContent_MakeAvailable(googletest)

set(test_libraries pthread gtest gmock gmock_main)

# Setup libvmi

option(ENABLE_STATIC "Build libvmi static library" OFF)
option(BUILD_EXAMPLES "Build the examples" OFF)

FetchContent_Declare(
        libvmi
        GIT_REPOSITORY https://github.com/libvmi/libvmi
        GIT_TAG b34cb61
)
FetchContent_MakeAvailable(libvmi)

include_directories(BEFORE SYSTEM ${libvmi_SOURCE_DIR})

# Setup yaml-cpp

set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "")
FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG yaml-cpp-0.7.0
)
FetchContent_MakeAvailable(yaml-cpp)
set_property(TARGET yaml-cpp PROPERTY POSITION_INDEPENDENT_CODE TRUE)

# Setup Boost DI

FetchContent_Declare(
        boost-di
        GIT_REPOSITORY https://github.com/boost-ext/di.git
        GIT_TAG v1.2.0
)
FetchContent_MakeAvailable(boost-di)
include_directories(BEFORE SYSTEM ${boost-di_SOURCE_DIR}/include)

# Rust grpc server

add_subdirectory(rust_src)
include_directories(${CMAKE_BINARY_DIR}/rust_src)
list(APPEND libraries rust_grpc_server)

# Discover project source files

file(GLOB_RECURSE source_files "src/*.h" "src/*.cpp")
list(FILTER source_files EXCLUDE REGEX "main\.cpp")
file(GLOB_RECURSE test_files "test/*.h" "test/*.cpp")

# End variable section

add_executable(vmicore "src/main.cpp" ${source_files})
set_property(TARGET vmicore PROPERTY POSITION_INDEPENDENT_CODE TRUE)
target_compile_options(vmicore PRIVATE ${core_compile_flags} ${extra_compile_flags})
target_compile_definitions(vmicore PRIVATE "INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
target_link_libraries(vmicore ${libraries} ${extra_libraries})

add_executable(vmicore-test EXCLUDE_FROM_ALL ${source_files} ${test_files})
target_compile_options(vmicore-test PRIVATE -g -O0 --coverage)
target_link_options(vmicore-test PRIVATE --coverage)
target_link_libraries(vmicore-test ${libraries} ${test_libraries})

# Setup test discovery

include(GoogleTest)

enable_testing()
gtest_discover_tests(vmicore-test)
