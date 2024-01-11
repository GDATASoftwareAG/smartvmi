include(CheckPIESupported)
check_pie_supported()

add_library(inmemoryscanner-obj OBJECT
        Config.cpp
        Dumping.cpp
        InMemory.cpp
        OutputXML.cpp
        Scanner.cpp
        YaraInterface.cpp)
target_compile_features(inmemoryscanner-obj PUBLIC cxx_std_20)
set_target_properties(inmemoryscanner-obj PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(inmemoryscanner-obj INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

include(FindPkgConfig)

pkg_check_modules(YARA REQUIRED yara>=4)
target_link_libraries(inmemoryscanner-obj PUBLIC ${YARA_LINK_LIBRARIES})

if (${YARA_VERSION} VERSION_GREATER_EQUAL 4.1)
    target_compile_definitions(inmemoryscanner-obj PRIVATE LIBYARA_4_1)
endif ()

pkg_check_modules(TCLAP REQUIRED tclap>=1.2)

include(FetchContent)

# Add public vmicore headers

add_subdirectory("${VMICORE_DIRECTORY_ROOT}/src/include" "${CMAKE_CURRENT_BINARY_DIR}/vmicore-public-headers")
target_link_libraries(inmemoryscanner-obj PUBLIC vmicore-public-headers)

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
target_compile_definitions(inmemoryscanner-obj PUBLIC YAML_CPP_SUPPORT)
target_link_libraries(inmemoryscanner-obj PUBLIC yaml-cpp)

# Setup fmt library

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 8.1.1
)
FetchContent_MakeAvailable(fmt)
target_link_libraries(inmemoryscanner-obj PUBLIC fmt-header-only)
