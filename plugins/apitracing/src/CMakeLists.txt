cmake_minimum_required(VERSION 3.16)
project(apitracing)

add_subdirectory(lib)

add_library(apitracing MODULE)
target_link_libraries(apitracing apitracing-obj)

# Installation

include(GNUInstallDirs)

install(TARGETS apitracing
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
