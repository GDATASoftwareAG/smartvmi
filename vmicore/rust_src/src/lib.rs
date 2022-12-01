pub mod bridge;
mod console_logger;
mod grpc_log_service;
mod grpc_logger;
pub mod grpc_server;
mod grpc_vmi_service;
mod logging;
mod unix_socket;

pub mod pkg {
    #![allow(clippy::derive_partial_eq_without_eq)]
    pub mod logging {
        pub mod service {
            pub mod v1 {
                tonic::include_proto!("pkg.logging.service.v1");
            }
        }
        pub mod api {
            pub mod v1 {
                tonic::include_proto!("pkg.logging.api.v1");
            }
        }
    }
    pub mod vmi {
        pub mod v1 {
            tonic::include_proto!("pkg.vmi.v1");
        }
    }
}
