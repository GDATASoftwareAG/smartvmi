cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(page_protection OBJECT PageProtection.cpp)
target_include_directories(page_protection INTERFACE windows/)
