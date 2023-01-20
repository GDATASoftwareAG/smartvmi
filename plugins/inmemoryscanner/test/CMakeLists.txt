add_executable(inmemoryscanner-test
        FakeYaraInterface.cpp
        Scanner_unittest.cpp
        YaraInterface_unittest.cpp)
target_link_libraries(inmemoryscanner-test PRIVATE inmemoryscanner-obj)

# Setup bundled google test framework

find_package(GTest CONFIG REQUIRED)
target_link_libraries(inmemoryscanner-test PRIVATE GTest::gmock GTest::gtest GTest::gmock_main)

# Add VmiCore public test headers

add_subdirectory("${VMICORE_DIRECTORY_ROOT}/test/include" "${CMAKE_CURRENT_BINARY_DIR}/vmicore-public-test-headers")
target_link_libraries(inmemoryscanner-test PRIVATE vmicore-public-test-headers)

# Setup test discovery

include(GoogleTest)
gtest_discover_tests(inmemoryscanner-test)
