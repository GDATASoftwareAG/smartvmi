cmake_minimum_required(VERSION 3.16)
project(inmemoryscanner)

add_subdirectory(lib)

add_library(inmemoryscanner MODULE)
target_link_libraries(inmemoryscanner inmemoryscanner-obj)

# Installation

include(GNUInstallDirs)

install(TARGETS inmemoryscanner
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
