add_executable(inmemoryscanner-test
        FakeYaraInterface.cpp
        Scanner_unittest.cpp
        YaraInterface_unittest.cpp)
target_link_libraries(inmemoryscanner-test inmemoryscanner-obj pthread)

# Setup bundled google test framework

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)
option(INSTALL_GTEST "" OFF)
FetchContent_MakeAvailable(googletest)
target_link_libraries(inmemoryscanner-test gtest gmock gmock_main)

# Add VmiCore public test headers

add_subdirectory("${VMICORE_DIRECTORY_ROOT}/test/include" "${CMAKE_CURRENT_BINARY_DIR}/vmicore-public-test-headers")
target_link_libraries(inmemoryscanner-test vmicore-public-test-headers)

# Setup test discovery

include(GoogleTest)
gtest_discover_tests(inmemoryscanner-test)
