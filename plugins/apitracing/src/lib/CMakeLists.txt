include(CheckPIESupported)
check_pie_supported()

add_library(apitracing-obj OBJECT
        ApiTracing.cpp
        config/Config.cpp
        config/FunctionDefinitions.cpp
        os/windows/Library.cpp
        os/Extractor.cpp
        FunctionHook.cpp
        TracedProcess.cpp
        TracedProcessFactory.cpp
        Tracer.cpp)
target_compile_features(apitracing-obj PUBLIC cxx_std_20)
set_target_properties(apitracing-obj PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(apitracing-obj INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

include(FindPkgConfig)
pkg_check_modules(TCLAP REQUIRED tclap>=1.2)

include(FetchContent)

# Setup fmt library as it is not available yet in gnu stdlib

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 8.1.1
)
FetchContent_MakeAvailable(fmt)
target_link_libraries(apitracing-obj PUBLIC fmt-header-only)

# Setup yaml-cpp

FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG yaml-cpp-0.7.0
)
option(YAML_BUILD_SHARED_LIBS "" OFF)
option(YAML_CPP_BUILD_TOOLS "" OFF)
FetchContent_MakeAvailable(yaml-cpp)
set_property(TARGET yaml-cpp PROPERTY POSITION_INDEPENDENT_CODE TRUE)
target_compile_definitions(apitracing-obj PUBLIC YAML_CPP_SUPPORT)
target_link_libraries(apitracing-obj PUBLIC yaml-cpp)

# Setup json-cpp

FetchContent_Declare(
        jsoncpp
        GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
        GIT_TAG 1.9.5
)
option(JSONCPP_WITH_TESTS "" OFF)
option(JSONCPP_WITH_POST_BUILD_UNITTEST "" OFF)
option(JSONCPP_WITH_EXAMPLE "" OFF)
FetchContent_MakeAvailable(jsoncpp)
set_property(TARGET jsoncpp_static PROPERTY POSITION_INDEPENDENT_CODE TRUE)
target_link_libraries(apitracing-obj PUBLIC jsoncpp_static)

# Add public vmicore headers

add_subdirectory("${VMICORE_DIRECTORY_ROOT}/src/include" "${CMAKE_CURRENT_BINARY_DIR}/vmicore-public-headers")
target_link_libraries(apitracing-obj PUBLIC vmicore-public-headers)

