use std::error::Error;

use async_std::channel::Sender;
use async_std::task;
use chrono::Utc;

use crate::bridge::ffi::Level;
use crate::pkg::logging::api::v1::{LogField, LogLevel, LogMessage};

#[derive(thiserror::Error, Debug)]
pub enum LogError {
    #[error("logging is closed")]
    LoggingClosedError,
}

pub struct GrpcLogger {
    sender: Sender<LogMessage>,
    base_fields: Vec<LogField>,
    log_level: Level,
    enable_debug: bool,
}

impl GrpcLogger {
    pub fn new(log_level: Level, enable_debug: bool, sender: Sender<LogMessage>, fields: Vec<LogField>) -> GrpcLogger {
        GrpcLogger {
            log_level,
            sender,
            enable_debug,
            base_fields: fields,
        }
    }

    pub fn bind(&mut self, log_fields: &[Box<LogField>]) {
        self.base_fields.extend(log_fields.iter().cloned().map(|v| *v));
    }

    pub fn log(self: &GrpcLogger, level: Level, message: &str, fields: &[Box<LogField>]) -> Result<(), Box<dyn Error>> {
        if self.sender.is_closed() {
            return Err(Box::new(LogError::LoggingClosedError));
        }
        if level < self.log_level {
            return Ok(());
        }

        let combined_fields: Vec<LogField> = self
            .base_fields
            .iter()
            .cloned()
            .chain(fields.iter().cloned().map(|v| *v))
            .collect();

        if self.enable_debug {
            println!(
                "{} {} {} {:?}",
                Utc::now().to_rfc3339(),
                level,
                message,
                combined_fields
            );
        }

        task::block_on(self.sender.send(LogMessage {
            time_unix: Utc::now().timestamp() as u64,
            level: Into::<LogLevel>::into(level) as i32,
            msg: message.to_string(),
            fields: combined_fields,
        }))?;
        Ok(())
    }
}
