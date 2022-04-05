fn main() -> Result<(), Box<dyn std::error::Error>> {
    tonic_build::configure().build_client(false).compile(
        &[
            "./protos/pkg/logging/service/log_svc.proto",
            "./protos/pkg/vmi/vmi_svc.proto",
        ],
        &["./protos"],
    )?;
    println!("cargo:rerun-if-changed=protos/");

    let _build = cxx_build::bridge("src/bridge.rs");
    println!("cargo:rerun-if-changed=src/bridge.rs");

    Ok(())
}
