mod bridge;
mod console_logger;
mod grpc_log_service;
mod grpc_logger;
mod grpc_server;
mod grpc_vmi_service;
mod logging;
mod unix_socket;

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
