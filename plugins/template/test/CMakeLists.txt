add_executable(template-test
        TemplateCode_unittest.cpp)
target_compile_options(template-test PRIVATE -Wno-missing-field-initializers)
target_link_libraries(template-test template-obj pthread)

# Setup bundled google test framework

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main
)
option(INSTALL_GTEST "" OFF)
FetchContent_MakeAvailable(googletest)
target_link_libraries(template-test gtest gmock gmock_main)

# Add VmiCore public test headers

add_subdirectory("${VMICORE_DIRECTORY_ROOT}/test/include" "${CMAKE_CURRENT_BINARY_DIR}/vmicore-public-test-headers")
target_link_libraries(template-test vmicore-public-test-headers)

# Setup test discovery

include(GoogleTest)
gtest_discover_tests(template-test)
