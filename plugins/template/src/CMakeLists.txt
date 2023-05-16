cmake_minimum_required(VERSION 3.16)
project(template)

add_subdirectory(lib)

add_library(template MODULE)
target_link_libraries(template template-obj)

# Installation

include(GNUInstallDirs)

install(TARGETS template
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
