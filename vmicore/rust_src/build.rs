fn main() -> Result<(), Box<dyn std::error::Error>> {
    tonic_build::configure().build_client(false).compile(
        &[
            "./protos/pkg/logging/service/v1/log_svc.proto",
            "./protos/pkg/vmi/v1/vmi_svc.proto",
        ],
        &["./protos"],
    )?;
    println!("cargo:rerun-if-changed=protos/");

    let _build = cxx_build::bridge("src/bridge.rs");
    println!("cargo:rerun-if-changed=src/bridge.rs");

    Ok(())
}
