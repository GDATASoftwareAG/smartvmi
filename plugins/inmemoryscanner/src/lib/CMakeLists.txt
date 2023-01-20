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

# Add public vmicore headers

add_subdirectory("${VMICORE_DIRECTORY_ROOT}/src/include" "${CMAKE_CURRENT_BINARY_DIR}/vmicore-public-headers")
target_link_libraries(inmemoryscanner-obj PUBLIC vmicore-public-headers)

# Setup tclap

find_path(TCLAP_INCLUDE_DIRS "tclap/Arg.h" REQUIRED)
target_include_directories(inmemoryscanner-obj PUBLIC ${TCLAP_INCLUDE_DIRS})

# Setup yara

find_package(unofficial-libyara CONFIG REQUIRED)
target_link_libraries(inmemoryscanner-obj PUBLIC unofficial::libyara::libyara)

# Setup yaml-cpp

find_package(yaml-cpp CONFIG REQUIRED)
target_link_libraries(inmemoryscanner-obj PUBLIC yaml-cpp::yaml-cpp)
target_compile_definitions(inmemoryscanner-obj PUBLIC YAML_CPP_SUPPORT)

# Setup fmt library

find_package(fmt CONFIG REQUIRED)
target_link_libraries(inmemoryscanner-obj PUBLIC fmt::fmt)
