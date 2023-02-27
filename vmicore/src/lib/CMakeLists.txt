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
        vmi/RegisterEventSupervisor.cpp
        vmi/Event.cpp
        vmi/InterruptEventSupervisor.cpp
        vmi/InterruptGuard.cpp
        vmi/LibvmiInterface.cpp
        vmi/SingleStepSupervisor.cpp
        vmi/VmiInitData.cpp
        vmi/VmiInitError.cpp)
target_compile_features(vmicore-lib PUBLIC cxx_std_20)
set_target_properties(vmicore-lib PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_link_libraries(vmicore-lib PUBLIC vmicore-public-headers dl)
target_include_directories(vmicore-lib INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

# Rust grpc server

add_subdirectory(../../rust_src rust_src)
target_link_libraries(vmicore-lib PUBLIC cxx_rust_part)

include(FetchContent)

# Setup libvmi

find_package(libvmi CONFIG REQUIRED)
# json-c is a re-exported dependency of libvmi
target_link_libraries(vmicore-lib PUBLIC libvmi::libvmi)

# Setup yaml-cpp

find_package(yaml-cpp CONFIG REQUIRED)
target_link_libraries(vmicore-lib PUBLIC yaml-cpp)
# Enable optional feature from plugin config header in order to be able to implement it
target_compile_definitions(vmicore-lib PUBLIC YAML_CPP_SUPPORT)

# Setup Guideline Support Library

find_package(Microsoft.GSL CONFIG REQUIRED)
target_link_libraries(vmicore-lib PUBLIC Microsoft.GSL::GSL)

# Setup fmt library as it is not available yet in gnu stdlib

find_package(fmt CONFIG REQUIRED)
target_link_libraries(vmicore-lib PUBLIC fmt::fmt)
