cmake_minimum_required(VERSION 3.16)
project(vmicore)

include(CheckPIESupported)
check_pie_supported()

include(FindPkgConfig)
pkg_check_modules(TCLAP REQUIRED tclap>=1.2)

add_subdirectory(include)
add_subdirectory(lib)

add_executable(vmicore main.cpp Cmdline.cpp)
target_compile_features(vmicore PRIVATE cxx_std_20)
set_target_properties(vmicore PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_link_libraries(vmicore vmicore-lib)

# Setup Boost DI

FetchContent_Declare(
        boost-di
        GIT_REPOSITORY https://github.com/boost-ext/di.git
        GIT_TAG v1.2.0
)
FetchContent_MakeAvailable(boost-di)
target_link_libraries(vmicore Boost.DI)

# Installation

include(GNUInstallDirs)

target_compile_definitions(vmicore PRIVATE SYSCONF_DIR="${CMAKE_INSTALL_FULL_SYSCONFDIR}")

install(TARGETS vmicore)
install(FILES ../configurations/configuration.yml
        TYPE SYSCONF
        RENAME vmicore.yml)
