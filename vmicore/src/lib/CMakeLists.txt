include(CheckPIESupported)
check_pie_supported()

add_library(vmicore-lib OBJECT
        GlobalControl.cpp
        VmiHub.cpp
        config/ConfigYAMLParser.cpp
        config/PluginConfig.cpp
        io/console/ConsoleLogger.cpp
        io/console/ConsoleLoggerBuilder.cpp
        io/file/LegacyLogging.cpp
        io/grpc/GRPCLogger.cpp
        io/grpc/GRPCServer.cpp
        os/PageProtection.cpp
        os/windows/ActiveProcessesSupervisor.cpp
        os/windows/KernelAccess.cpp
        os/windows/KernelOffsets.cpp
        os/windows/SystemEventSupervisor.cpp
        os/windows/VadTreeWin10.cpp
        os/linux/ActiveProcessesSupervisor.cpp
        os/linux/MMExtractor.cpp
        os/linux/PathExtractor.cpp
        os/linux/SystemEventSupervisor.cpp
        plugins/PluginSystem.cpp
        vmi/Breakpoint.cpp
        vmi/Event.cpp
        vmi/InterruptEventSupervisor.cpp
        vmi/InterruptGuard.cpp
        vmi/LibvmiInterface.cpp
        vmi/SingleStepSupervisor.cpp
        vmi/VmiInitData.cpp
        vmi/VmiInitError.cpp
        vmi/VmiUnicodeStruct.cpp)
target_compile_features(vmicore-lib PUBLIC cxx_std_20)
set_target_properties(vmicore-lib PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_link_libraries(vmicore-lib PUBLIC vmicore-public-headers dl)
target_include_directories(vmicore-lib INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

# Rust grpc server

add_subdirectory(../../rust_src rust_src)
target_link_libraries(vmicore-lib PUBLIC rust_grpc_server)

include(FetchContent)

# Setup libvmi

FetchContent_Declare(
        libvmi
        GIT_REPOSITORY https://github.com/GDATASoftwareAG/libvmi
        GIT_TAG test
)
option(ENABLE_STATIC "" OFF)
option(BUILD_EXAMPLES "" OFF)
FetchContent_MakeAvailable(libvmi)
target_include_directories(vmicore-lib BEFORE PUBLIC ${libvmi_SOURCE_DIR})
target_link_libraries(vmicore-lib PUBLIC vmi_shared)

# Setup yaml-cpp

FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG yaml-cpp-0.7.0
)
option(YAML_BUILD_SHARED_LIBS "" OFF)
option(YAML_CPP_BUILD_TOOLS "" OFF)
FetchContent_MakeAvailable(yaml-cpp)
set_target_properties(yaml-cpp PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
# Enable optional feature from plugin config header in order to be able to implement it
target_compile_definitions(vmicore-lib PUBLIC YAML_CPP_SUPPORT)
target_link_libraries(vmicore-lib PUBLIC yaml-cpp)

# Setup Guideline Support Library

FetchContent_Declare(
        gsl
        GIT_REPOSITORY https://github.com/microsoft/GSL.git
        GIT_TAG v4.0.0
)
FetchContent_MakeAvailable(gsl)
target_link_libraries(vmicore-lib PUBLIC GSL)

# Setup fmt library as it is not available yet in gnu stdlib

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 8.1.1
)
FetchContent_MakeAvailable(fmt)
target_link_libraries(vmicore-lib PUBLIC fmt-header-only)
