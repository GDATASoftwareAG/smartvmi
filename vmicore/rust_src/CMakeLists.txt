include(FetchContent)
FetchContent_Declare(
        Corrosion
        GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
        GIT_TAG master
)
FetchContent_MakeAvailable(Corrosion)

corrosion_import_crate(MANIFEST_PATH Cargo.toml)
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    corrosion_set_env_vars(rust_grpc_server CXXFLAGS=-stdlib=libc++)
endif ()
corrosion_add_cxxbridge(cxx_rust_part CRATE rust_grpc_server FILES bridge.rs)
set_property(TARGET cxx_rust_part PROPERTY POSITION_INDEPENDENT_CODE TRUE)
