cmake_minimum_required(VERSION 3.16)
project(vmicore)


# Options

set(PROGRAM_VERSION "0.0.0" CACHE STRING "Program version.")
set(PROGRAM_BUILD_NUMBER "testbuild" CACHE STRING "Build number.")
option(TRACE_MODE "Include extra tracing output" OFF)
option(VMI_SANITIZERS "Build with sanitizers." OFF)

# Variable section

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(core_compile_flags -m64 -Wall)
set(extra_compile_flags -Wunused -Wunreachable-code -Wextra -Wpedantic -Wno-dollar-in-identifier-extension)

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

add_compile_definitions(PROGRAM_VERSION="${PROGRAM_VERSION}" BUILD_VERSION="${PROGRAM_BUILD_NUMBER}")
if (TRACE_MODE)
    add_compile_definitions(TRACE_MODE)
endif ()

# Check if tclap is present

include(FindPkgConfig)

pkg_check_modules(TCLAP REQUIRED tclap>=1.2)

include(FetchContent)

# Setup bundled google test framework

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)
option(INSTALL_GTEST "" OFF)
FetchContent_MakeAvailable(googletest)

# Setup libvmi

FetchContent_Declare(
        libvmi
        GIT_REPOSITORY https://github.com/GDATASoftwareAG/libvmi
        GIT_TAG test
)
option(ENABLE_STATIC "" OFF)
option(BUILD_EXAMPLES "" OFF)
FetchContent_MakeAvailable(libvmi)

include_directories(BEFORE SYSTEM ${libvmi_SOURCE_DIR})

# Setup yaml-cpp

FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG yaml-cpp-0.7.0
)
option(YAML_BUILD_SHARED_LIBS "" OFF)
option(YAML_CPP_BUILD_TOOLS "" OFF)
FetchContent_MakeAvailable(yaml-cpp)
set_property(TARGET yaml-cpp PROPERTY POSITION_INDEPENDENT_CODE TRUE)
# Enable optional feature from plugin config header in order to be able to implement it
add_compile_definitions(YAML_CPP_SUPPORT)

# Setup Boost DI

FetchContent_Declare(
        boost-di
        GIT_REPOSITORY https://github.com/boost-ext/di.git
        GIT_TAG v1.2.0
)
FetchContent_MakeAvailable(boost-di)
include_directories(BEFORE SYSTEM ${boost-di_SOURCE_DIR}/include)

# Setup Guideline Support Library

FetchContent_Declare(
        gsl
        GIT_REPOSITORY https://github.com/microsoft/GSL.git
        GIT_TAG v4.0.0
)
FetchContent_MakeAvailable(gsl)

# Setup fmt library as it is not available yet in gnu stdlib

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 8.1.1
)
FetchContent_MakeAvailable(fmt)

# Rust grpc server

add_subdirectory(rust_src)
include_directories(${CMAKE_BINARY_DIR}/rust_src)

# Public header files

add_subdirectory(include)

# Project source files

set(source_files
        src/GlobalControl.cpp
        src/VmiHub.cpp
        src/config/ConfigYAMLParser.cpp
        src/config/PluginConfig.cpp
        src/io/console/ConsoleLogger.cpp
        src/io/console/ConsoleLoggerBuilder.cpp
        src/io/file/LegacyLogging.cpp
        src/io/grpc/GRPCLogger.cpp
        src/io/grpc/GRPCServer.cpp
        src/os/PageProtection.cpp
        src/os/windows/ActiveProcessesSupervisor.cpp
        src/os/windows/KernelAccess.cpp
        src/os/windows/KernelOffsets.cpp
        src/os/windows/SystemEventSupervisor.cpp
        src/os/windows/VadTreeWin10.cpp
        src/os/linux/ActiveProcessesSupervisor.cpp
        src/os/linux/MMExtractor.cpp
        src/os/linux/PathExtractor.cpp
        src/os/linux/SystemEventSupervisor.cpp
        src/plugins/PluginSystem.cpp
        src/vmi/InterruptEvent.cpp
        src/vmi/InterruptFactory.cpp
        src/vmi/InterruptGuard.cpp
        src/vmi/LibvmiInterface.cpp
        src/vmi/SingleStepSupervisor.cpp
        src/vmi/VmiInitData.cpp
        src/vmi/VmiInitError.cpp)

set(test_files
        test/os/windows/ActiveProcessesSupervisor_UnitTest.cpp
        test/os/windows/KernelAccess_UnitTest.cpp
        test/os/windows/SystemEventSupervisor_UnitTest.cpp
        test/plugins/PluginSystem_UnitTest.cpp
        test/vmi/InterruptEvent_UnitTest.cpp
        test/vmi/LibvmiInterface_UnitTest.cpp
        test/vmi/SingleStepSupervisor_UnitTest.cpp)

# Link libraries

set(libraries rust_grpc_server vmicore_public_headers vmi_shared dl yaml-cpp GSL fmt-header-only)
set(test_libraries pthread gtest gmock gmock_main)

# End variable section

add_executable(vmicore "src/main.cpp" "src/Cmdline.cpp" ${source_files})
set_property(TARGET vmicore PROPERTY POSITION_INDEPENDENT_CODE TRUE)
target_compile_options(vmicore PRIVATE ${core_compile_flags} ${extra_compile_flags})
target_compile_definitions(vmicore PRIVATE "INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
target_link_libraries(vmicore ${libraries} ${extra_libraries})

add_executable(vmicore-test ${source_files} ${test_files})
target_compile_options(vmicore-test PRIVATE -g -O0 --coverage)
target_link_options(vmicore-test PRIVATE --coverage)
target_link_libraries(vmicore-test ${libraries} ${test_libraries})

# Setup test discovery

include(GoogleTest)

enable_testing()
gtest_discover_tests(vmicore-test)
