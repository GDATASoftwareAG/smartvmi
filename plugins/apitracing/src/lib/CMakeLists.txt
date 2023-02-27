include(CheckPIESupported)
check_pie_supported()

add_library(apitracing-obj OBJECT
        ApiTracing.cpp
        Tracer.cpp
        config/Config.cpp
        config/FunctionDefinitions.cpp
        os/windows/Library.cpp
        FunctionHook.cpp
        os/Extractor.cpp)
target_compile_features(apitracing-obj PUBLIC cxx_std_20)
set_target_properties(apitracing-obj PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(apitracing-obj INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

# Setup tclap

find_package(tclap CONFIG REQUIRED)
target_link_libraries(apitracing-obj PUBLIC tclap::tclap)

# Setup fmt library as it is not available yet in gnu stdlib

find_package(fmt CONFIG REQUIRED)
target_link_libraries(apitracing-obj PUBLIC fmt::fmt)

# Setup yaml-cpp

find_package(yaml-cpp CONFIG REQUIRED)
target_link_libraries(apitracing-obj PUBLIC yaml-cpp)
target_compile_definitions(apitracing-obj PUBLIC YAML_CPP_SUPPORT)

# Add public vmicore headers

add_subdirectory("${VMICORE_DIRECTORY_ROOT}/src/include" "${CMAKE_CURRENT_BINARY_DIR}/vmicore-public-headers")
target_link_libraries(apitracing-obj PUBLIC vmicore-public-headers)
