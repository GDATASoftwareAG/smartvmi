add_executable(vmicore-test
        lib/os/windows/ActiveProcessesSupervisor_UnitTest.cpp
        lib/os/windows/KernelAccess_UnitTest.cpp
        lib/os/windows/SystemEventSupervisor_UnitTest.cpp
        lib/plugins/PluginSystem_UnitTest.cpp
        lib/vmi/ContextSwitchHandler_UnitTest.cpp
        lib/vmi/InterruptEventSupervisor_UnitTest.cpp
        lib/vmi/LibvmiInterface_UnitTest.cpp
        lib/vmi/SingleStepSupervisor_UnitTest.cpp)
target_compile_options(vmicore-test PRIVATE -Wno-missing-field-initializers)
target_link_libraries(vmicore-test vmicore-lib pthread)

# Setup bundled google test framework

find_package(GTest CONFIG REQUIRED)
target_link_libraries(vmicore-test gtest::gtest)

# Public test headers

add_subdirectory(include)
target_link_libraries(vmicore-test vmicore-public-test-headers)

# Dummy executable for testing potentially unused mocks

add_subdirectory(mocks)

# Setup test discovery

include(GoogleTest)
gtest_discover_tests(vmicore-test)
