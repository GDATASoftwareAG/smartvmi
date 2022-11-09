cmake_minimum_required(VERSION 3.16)
project(inmemoryscanner)

set(PROGRAM_VERSION "2.5.0" CACHE STRING "Program version.")
set(PROGRAM_BUILD_NUMBER "testbuild" CACHE STRING "Build number.")
set(VMICORE_DIRECTORY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../vmicore" CACHE PATH "Path to directory root of VMICore project.")

# Dependencies

include(FindPkgConfig)

pkg_check_modules(YARA REQUIRED yara>=4)

pkg_check_modules(TCLAP REQUIRED tclap>=1.2)

include(FetchContent)

# Setup bundled google test framework

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.11.0
)
option(INSTALL_GTEST "" OFF)
FetchContent_MakeAvailable(googletest)

# Setup yaml-cpp

FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG yaml-cpp-0.7.0
)
option(YAML_BUILD_SHARED_LIBS "" OFF)
option(YAML_CPP_BUILD_TOOLS "" OFF)
FetchContent_MakeAvailable(yaml-cpp)
set_property(TARGET yaml-cpp PROPERTY POSITION_INDEPENDENT_CODE TRUE)
add_compile_definitions(YAML_CPP_SUPPORT)

# Add public vmicore headers

add_subdirectory("${VMICORE_DIRECTORY_ROOT}/src/include" "${CMAKE_CURRENT_BINARY_DIR}/vmicore-public-headers")

# Variable section

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(compile_flags -Wunused -Wunreachable-code -Wall -Wextra -Wpedantic)
set(libraries vmicore-public-headers ${YARA_LINK_LIBRARIES} yaml-cpp)

add_compile_definitions(BUILD_VERSION="${PROGRAM_BUILD_NUMBER}" PLUGIN_VERSION="${PROGRAM_VERSION}")

# End variable section

set(source_files
        src/Config.cpp
        src/Dumping.cpp
        src/InMemory.cpp
        src/OutputXML.cpp
        src/Scanner.cpp
        src/Yara.cpp)

set(test_files
        test/FakeYara.cpp
        test/Scanner_unittest.cpp)

add_library(inmemoryscanner MODULE ${source_files})

target_compile_options(inmemoryscanner PRIVATE ${compile_flags})
target_link_libraries(inmemoryscanner ${libraries})

add_executable(inmemoryscanner-test ${source_files} ${test_files})
target_compile_options(inmemoryscanner-test PRIVATE ${compile_flags} -g -O0 --coverage)
target_link_options(inmemoryscanner-test PRIVATE --coverage)
target_link_libraries(inmemoryscanner-test ${libraries} pthread gtest gmock gmock_main)

# Add VmiCore public test headers

add_subdirectory("${VMICORE_DIRECTORY_ROOT}/test/include" "${CMAKE_CURRENT_BINARY_DIR}/vmicore-public-test-headers")
target_link_libraries(inmemoryscanner-test vmicore-public-test-headers)

# Setup test discovery

include(GoogleTest)

enable_testing()
gtest_discover_tests(inmemoryscanner-test)
