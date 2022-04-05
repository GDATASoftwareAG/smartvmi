if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_CMD cargo build)
    set(TARGET_DIR "debug")
else ()
    set(CARGO_CMD cargo build --release)
    set(TARGET_DIR "release")
endif ()

set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES "${TARGET_DIR};cxxbridge")

set(RUST_TARGET_NAME rust_grpc_server)
set(RUST_LIB "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}/lib${RUST_TARGET_NAME}.a")
set(RUST_BRIDGE_CXX "${CMAKE_CURRENT_BINARY_DIR}/cxxbridge/${RUST_TARGET_NAME}/src/bridge.rs.cc")
set(RUST_BRIDGE_H "${CMAKE_CURRENT_BINARY_DIR}/cxxbridge/${RUST_TARGET_NAME}/src/bridge.rs.h")

file(GLOB_RECURSE RUST_SOURCE_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.rs")

add_custom_command(
        OUTPUT ${RUST_BRIDGE_CXX} ${RUST_BRIDGE_H}
        COMMAND CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} RUSTFLAGS="${RUST_FLAGS}" ${CARGO_CMD}
        DEPENDS ${RUST_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)


add_library(${RUST_TARGET_NAME} STATIC ${RUST_BRIDGE_CXX})
target_include_directories(${RUST_TARGET_NAME} INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${RUST_TARGET_NAME} pthread dl ${RUST_LIB})
