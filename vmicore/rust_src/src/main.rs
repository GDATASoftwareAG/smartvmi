mod bridge;
mod console_logger;
mod grpc_log_service;
mod grpc_logger;
mod grpc_server;
mod grpc_vmi_service;
mod logging;
mod unix_socket;

use crate::grpc_server::new_server;

pub mod hive_operations {
    #![allow(clippy::derive_partial_eq_without_eq)]
    pub mod logging {
        pub mod service {
            tonic::include_proto!("hive_operations.logging.service");
        }
        tonic::include_proto!("hive_operations.logging");
    }
    pub mod vmi {
        tonic::include_proto!("hive_operations.vmi");
    }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let addr = "/mnt/rust/vmi.sock";
    let name = "FancyLogger";

    let srv = new_server(addr, false);
    let logger = srv.new_named_logger(name);
    let srv_handle = std::sync::Arc::new(srv);

    println!("Starting server...");
    let srv_handle_clone = srv_handle.clone();
    let join_handle = std::thread::spawn(move || {
        srv_handle_clone.start_server().unwrap();
        println!("Server start terminated");
    });

    println!("Writing log line");
    logger.log(bridge::ffi::Level::INFO, "Hello World", &[])?;

    println!("Wait for input...");
    std::io::stdin().read_line(&mut String::new())?;
    println!("Stopping server...");
    srv_handle.stop_server(2000);
    println!("Server stopped");
    join_handle.join().expect("Failed to join thread");

    Ok(())
}
