add_executable(template-test
        TemplateCode_unittest.cpp)
target_compile_options(template-test PRIVATE -Wno-missing-field-initializers)
target_link_libraries(template-test PRIVATE template-obj)

# Setup bundled google test framework

find_package(GTest CONFIG REQUIRED)
target_link_libraries(template-test PRIVATE GTest::gtest GTest::gmock GTest::gmock_main)

# Add VmiCore public test headers

add_subdirectory("${VMICORE_DIRECTORY_ROOT}/test/include" "${CMAKE_CURRENT_BINARY_DIR}/vmicore-public-test-headers")
target_link_libraries(template-test PRIVATE vmicore-public-test-headers)

# Setup test discovery

include(GoogleTest)
gtest_discover_tests(template-test)
