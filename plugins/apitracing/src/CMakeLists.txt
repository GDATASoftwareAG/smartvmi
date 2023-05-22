cmake_minimum_required(VERSION 3.16)
project(apitracing)

add_subdirectory(lib)

add_library(apitracing MODULE)
target_link_libraries(apitracing apitracing-obj)

# Installation

include(GNUInstallDirs)

target_compile_definitions(apitracing-obj PRIVATE CONFIG_DIR="${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/vmicore/plugins/${PROJECT_NAME}/")

install(TARGETS apitracing
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY ../configuration/
        DESTINATION ${CMAKE_INSTALL_LOCALSTATEDIR}/vmicore/plugins/${PROJECT_NAME}/)
