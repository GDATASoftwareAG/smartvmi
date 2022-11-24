use std::error::Error;

use chrono::Utc;

use crate::bridge::ffi::Level;
use crate::pkg::logging::api::v1::{log_field::Field, LogField};

pub struct ConsoleLoggerBuilder {
    log_level: Level,
}

pub fn new_console_logger_builder() -> Box<ConsoleLoggerBuilder> {
    Box::new(ConsoleLoggerBuilder { log_level: Level::INFO })
}

impl ConsoleLoggerBuilder {
    pub fn new_logger(&self) -> Box<ConsoleLogger> {
        Box::new(ConsoleLogger {
            base_fields: Vec::new(),
            log_level: self.log_level,
        })
    }

    pub fn new_named_logger(&self, name: &str) -> Box<ConsoleLogger> {
        let log_field = LogField {
            name: "logger".to_string(),
            field: Some(Field::StrField(name.to_string())),
        };
        Box::new(ConsoleLogger {
            base_fields: vec![log_field],
            log_level: self.log_level,
        })
    }

    pub fn set_log_level(&mut self, log_level: crate::bridge::ffi::Level) {
        self.log_level = log_level
    }
}

pub struct ConsoleLogger {
    base_fields: Vec<LogField>,
    log_level: Level,
}

impl ConsoleLogger {
    pub fn bind(&mut self, log_fields: &[Box<LogField>]) {
        self.base_fields.extend(log_fields.iter().cloned().map(|v| *v));
    }

    pub fn log(
        self: &ConsoleLogger,
        level: Level,
        message: &str,
        fields: &[Box<LogField>],
    ) -> Result<(), Box<dyn Error>> {
        if level < self.log_level {
            return Ok(());
        }

        let combined_fields: Vec<LogField> = self
            .base_fields
            .iter()
            .cloned()
            .chain(fields.iter().cloned().map(|v| *v))
            .collect();
        println!(
            "{} {} {} {}",
            Utc::now().to_rfc3339(),
            level,
            message,
            combined_fields
                .iter()
                .fold(String::new(), |acc, el| acc + "\n\t" + &el.to_string())
        );
        Ok(())
    }
}
