cmake_minimum_required(VERSION 3.16)
project(vmicore)

include(CheckPIESupported)
check_pie_supported()

add_subdirectory(include)
add_subdirectory(lib)

add_executable(vmicore main.cpp Cmdline.cpp)
target_compile_features(vmicore PRIVATE cxx_std_20)
set_target_properties(vmicore PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_link_libraries(vmicore vmicore-lib)

# Setup tclap

find_path(TCLAP_INCLUDE_DIRS "tclap/Arg.h" REQUIRED)
target_include_directories(vmicore PRIVATE ${TCLAP_INCLUDE_DIRS})

# Setup Boost DI

find_path(BEXT_DI_INCLUDE_DIRS "boost/di.hpp")
target_include_directories(vmicore PRIVATE ${BEXT_DI_INCLUDE_DIRS})

# Installation

include(GNUInstallDirs)

target_compile_definitions(vmicore PRIVATE SYSCONF_DIR="${CMAKE_INSTALL_FULL_SYSCONFDIR}")

install(TARGETS vmicore)
install(FILES ../configurations/configuration.yml
        TYPE SYSCONF
        RENAME vmicore.yml)
