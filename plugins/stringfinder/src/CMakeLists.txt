cmake_minimum_required(VERSION 3.16)
project(stringfinder)

add_subdirectory(lib)

add_library(stringfinder MODULE)
target_link_libraries(stringfinder stringfinder-obj)

# Installation

include(GNUInstallDirs)

install(TARGETS stringfinder
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
