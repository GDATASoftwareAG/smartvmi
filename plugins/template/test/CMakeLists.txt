add_executable(template-test
        TemplateCode_unittest.cpp)
target_link_libraries(template-test PRIVATE template-obj pthread)

# Setup bundled google test framework

find_package(GTest CONFIG REQUIRED)
target_link_libraries(template-test PRIVATE gtest::gtest)

# Add VmiCore public test headers

add_subdirectory("${VMICORE_DIRECTORY_ROOT}/test/include" "${CMAKE_CURRENT_BINARY_DIR}/vmicore-public-test-headers")
target_link_libraries(template-test PRIVATE vmicore-public-test-headers)

# Setup test discovery

include(GoogleTest)
gtest_discover_tests(template-test)
