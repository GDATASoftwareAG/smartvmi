include(CheckPIESupported)
check_pie_supported()

add_library(stringfinder-obj OBJECT
        StringFinder.cpp
)
target_compile_features(stringfinder-obj PUBLIC cxx_std_20)
set_target_properties(stringfinder-obj PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(stringfinder-obj INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

include(FindPkgConfig)

pkg_check_modules(TCLAP REQUIRED tclap>=1.2)

include(FetchContent)

# Add public vmicore headers

add_subdirectory("${VMICORE_DIRECTORY_ROOT}/src/include" "${CMAKE_CURRENT_BINARY_DIR}/vmicore-public-headers")
target_link_libraries(stringfinder-obj PUBLIC vmicore-public-headers)
