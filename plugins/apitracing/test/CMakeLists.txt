add_executable(apitracing-test
        Config_UnitTest.cpp
        Extractor_Unittest.cpp
        FunctionDefinitions_UnitTest.cpp
        FunctionHook_UnitTest.cpp
        TracedProcess_UnitTest.cpp
        Tracer_UnitTest.cpp)
target_link_libraries(apitracing-test PRIVATE apitracing-obj)

# Setup bundled google test framework

find_package(GTest CONFIG REQUIRED)
target_link_libraries(apitracing-test PRIVATE GTest::gtest GTest::gmock GTest::gmock_main)

# Add VmiCore public test headers

add_subdirectory("${VMICORE_DIRECTORY_ROOT}/test/include" "${CMAKE_CURRENT_BINARY_DIR}/vmicore-public-test-headers")
target_link_libraries(apitracing-test PRIVATE vmicore-public-test-headers)

# Copy config files to bin directory

add_custom_command(
        TARGET apitracing-test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/testConfiguration.yaml
        ${CMAKE_CURRENT_BINARY_DIR}/testConfiguration.yaml)

add_custom_command(
        TARGET apitracing-test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/testFunctionDefinitions.yaml
        ${CMAKE_CURRENT_BINARY_DIR}/testFunctionDefinitions.yaml)


# Setup test discovery

include(GoogleTest)
gtest_discover_tests(apitracing-test)
