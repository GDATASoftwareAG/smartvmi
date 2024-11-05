fn main() -> Result<(), Box<dyn std::error::Error>> {
    tonic_build::configure().build_client(true).compile_protos(
        &[
            "./protos/pkg/logging/service/v1/log_svc.proto",
            "./protos/pkg/vmi/v1/vmi_svc.proto",
        ],
        &["./protos"],
    )?;
    println!("cargo:rerun-if-changed=protos/");

    Ok(())
}
