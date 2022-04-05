use crate::hive_operations::logging::LogField;

use crate::console_logger::{new_console_logger_builder, ConsoleLogger, ConsoleLoggerBuilder};
use crate::grpc_logger::GrpcLogger;
use crate::grpc_server::{new_server, GRPCServer};
use crate::logging::*;

#[cxx::bridge]
pub mod ffi {
    #[namespace = "logging"]
    #[derive(Debug, PartialOrd)]
    enum Level {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3,
    }

    #[namespace = "grpc"]
    enum ProcessState {
        Started,
        Terminated,
    }

    #[namespace = "logging"]
    extern "Rust" {
        fn convert_to_log_level(level: &CxxString) -> Result<Level>;

        type LogField;
        fn field_str(name: &CxxString, val: &CxxString) -> Box<LogField>;
        fn field_i64(name: &CxxString, val: i64) -> Box<LogField>;
        fn field_float64(name: &CxxString, val: f64) -> Box<LogField>;
        fn field_uint64(name: &CxxString, val: u64) -> Box<LogField>;
        fn field_bool(name: &CxxString, val: bool) -> Box<LogField>;
    }

    #[namespace = "logging::grpc"]
    extern "Rust" {
        type GrpcLogger;

        fn bind(self: &mut GrpcLogger, log_fields: &[Box<LogField>]);
        fn log(self: &GrpcLogger, level: Level, message: String, fields: &[Box<LogField>]) -> Result<()>;
    }

    #[namespace = "logging::console"]
    extern "Rust" {
        type ConsoleLogger;
        type ConsoleLoggerBuilder;

        fn new_console_logger_builder() -> Box<ConsoleLoggerBuilder>;
        fn set_log_level(self: &mut ConsoleLoggerBuilder, log_level: Level);

        fn new_logger(self: &ConsoleLoggerBuilder) -> Box<ConsoleLogger>;
        fn new_named_logger(self: &ConsoleLoggerBuilder, name: &CxxString) -> Box<ConsoleLogger>;
        fn bind(self: &mut ConsoleLogger, log_fields: &[Box<LogField>]);
        fn log(self: &ConsoleLogger, level: Level, message: String, fields: &[Box<LogField>]) -> Result<()>;
    }

    #[namespace = "grpc"]
    extern "Rust" {
        type GRPCServer;
        fn new_server(listenAddr: &CxxString, enable_debug: bool) -> Box<GRPCServer>;
        fn start_server(self: &mut GRPCServer) -> Result<()>;
        fn stop_server(self: &GRPCServer, timeout_millis: u64);

        fn new_logger(self: &GRPCServer) -> Box<GrpcLogger>;
        fn new_named_logger(self: &GRPCServer, name: &CxxString) -> Box<GrpcLogger>;
        fn set_log_level(self: &mut GRPCServer, log_level: Level);

        fn write_message_to_file(self: &GRPCServer, name: &CxxString, message: &CxxVector<u8>) -> Result<()>;
        fn send_process_event(
            self: &GRPCServer,
            process_state: ProcessState,
            process_name: &CxxString,
            process_id: u32,
            cr3: &CxxString,
        ) -> Result<()>;
        fn send_bsod_event(self: &GRPCServer, code: i64) -> Result<()>;
        fn send_ready_event(self: &GRPCServer) -> Result<()>;
        fn send_termination_event(self: &GRPCServer) -> Result<()>;
        fn send_error_event(self: &GRPCServer, message: &CxxString) -> Result<()>;
        fn send_in_mem_detection_event(self: &GRPCServer, message: &CxxString) -> Result<()>;
    }
}
