cmake_minimum_required(VERSION 3.16)
project(inmemoryscanner)

set(PROGRAM_VERSION "2.5.0" CACHE STRING "Program version.")
set(PROGRAM_BUILD_NUMBER "testbuild" CACHE STRING "Build number.")
set(VMICORE_DIRECTORY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../vmicore" CACHE PATH "Path to directory root of VMICore project.")

# Dependencies

include(FindPkgConfig)

pkg_check_modules(YARA REQUIRED yara>=4)

include(FetchContent)

# Setup bundled google test framework

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.11.0
)
set(googletest_BUILD_GMOCK ON)
set(googletest_BUILD_GTEST ON)
set(googletest_BUILD_SHARED_LIBS ON)
set(googletest_INSTALL_GTEST OFF)
set(googletest_INSTALL_GMOCK OFF)
FetchContent_MakeAvailable(googletest)

# Configure include directory for required VMICore plugin header files

include_directories("${VMICORE_DIRECTORY_ROOT}/src/plugins")

# Variable section

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(compile_flags -Wunused -Wunreachable-code -Wall -Wextra -Wpedantic)
set(libraries ${YARA_LINK_LIBRARIES})

add_definitions(-DBUILD_VERSION="${PROGRAM_BUILD_NUMBER}")

# End variable section

file(GLOB_RECURSE source_files "src/*.h" "src/*.cpp")
file(GLOB test_files "test/*.h" "test/*.cpp")

add_library(inmemoryscanner MODULE ${source_files})

target_compile_options(inmemoryscanner PRIVATE ${compile_flags})
target_link_libraries(inmemoryscanner ${libraries})

add_executable(inmemoryscanner-test ${source_files} ${test_files})

target_compile_options(inmemoryscanner-test PRIVATE ${compile_flags} -g -O0 --coverage)
target_link_options(inmemoryscanner-test PRIVATE --coverage)
target_link_libraries(inmemoryscanner-test ${libraries} pthread gtest gmock gmock_main)

# Setup test discovery

include(GoogleTest)

enable_testing()
gtest_discover_tests(inmemoryscanner-test)
