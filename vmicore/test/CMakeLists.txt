add_executable(vmicore-test
        lib/os/windows/ActiveProcessesSupervisor_UnitTest.cpp
        lib/os/windows/KernelAccess_UnitTest.cpp
        lib/os/windows/SystemEventSupervisor_UnitTest.cpp
        lib/plugins/PluginSystem_UnitTest.cpp
        lib/vmi/InterruptEventSupervisor_UnitTest.cpp
        lib/vmi/LibvmiInterface_UnitTest.cpp
        lib/vmi/SingleStepSupervisor_UnitTest.cpp)
target_link_libraries(vmicore-test vmicore-lib pthread)

# Setup bundled google test framework

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)
option(INSTALL_GTEST "" OFF)
FetchContent_MakeAvailable(googletest)
target_link_libraries(vmicore-test gtest gmock gmock_main)

# Public test headers

add_subdirectory(include)
target_link_libraries(vmicore-test vmicore-public-test-headers)

# Dummy executable for testing potentially unused mocks

add_subdirectory(mocks)

# Setup test discovery

include(GoogleTest)
gtest_discover_tests(vmicore-test)
