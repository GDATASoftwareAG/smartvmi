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

    pub fn bind(&mut self, fields: Vec<LogField>) {
        self.base_fields.extend(fields.into_iter());
    }

    pub fn clone_base_fields(&self, capacity: usize) -> Vec<LogField> {
        let mut cloned_fields = Vec::with_capacity(self.base_fields.len() + capacity);
        cloned_fields.extend(self.base_fields.iter().cloned());
        cloned_fields
    }

    pub fn log(&self, level: Level, message: &str, mut fields: Vec<LogField>) -> Result<(), Box<dyn Error>> {
        if self.sender.is_closed() {
            return Err(Box::new(LogError::LoggingClosedError));
        }
        if level < self.log_level {
            return Ok(());
        }

        fields.extend(self.base_fields.iter().cloned());

        self._log(level, message, fields)
    }

    pub fn log_no_base_fields(&self, level: Level, message: &str, fields: Vec<LogField>) -> Result<(), Box<dyn Error>> {
        if self.sender.is_closed() {
            return Err(Box::new(LogError::LoggingClosedError));
        }
        if level < self.log_level {
            return Ok(());
        }

        self._log(level, message, fields)
    }

    fn _log(&self, level: Level, message: &str, fields: Vec<LogField>) -> Result<(), Box<dyn Error>> {
        if self.enable_debug {
            println!("{} {} {} {:?}", Utc::now().to_rfc3339(), level, message, fields);
        }

        task::block_on(self.sender.send(LogMessage {
            time_unix: Utc::now().timestamp() as u64,
            level: LogLevel::from(level).into(),
            msg: message.to_string(),
            fields,
        }))?;

        Ok(())
    }
}
