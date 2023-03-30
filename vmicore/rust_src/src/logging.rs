use std::fmt;

use crate::{
    bridge::ffi::Level,
    pkg::logging::api::v1::{log_field::Field, LogField},
};

#[derive(thiserror::Error, Debug)]
pub enum LogError {
    #[error("log level not recognized")]
    LogLevelParseError,
}

pub fn convert_to_log_level(level: &str) -> Result<Level, Box<dyn std::error::Error>> {
    match level {
        "debug" => Ok(Level::DEBUG),
        "info" => Ok(Level::INFO),
        "warning" => Ok(Level::WARN),
        "error" => Ok(Level::ERROR),
        _ => Err(Box::new(LogError::LogLevelParseError)),
    }
}

pub fn add_field_str(fields: &mut Vec<LogField>, name: &str, val: &str) {
    fields.push(LogField {
        name: name.to_string(),
        field: Some(Field::StrField(val.to_string())),
    });
}

pub fn add_field_i64(fields: &mut Vec<LogField>, name: &str, val: i64) {
    fields.push(LogField {
        name: name.to_string(),
        field: Some(Field::IntField(val)),
    });
}

pub fn add_field_float64(fields: &mut Vec<LogField>, name: &str, val: f64) {
    fields.push(LogField {
        name: name.to_string(),
        field: Some(Field::FloatField(val)),
    });
}

pub fn add_field_uint64(fields: &mut Vec<LogField>, name: &str, val: u64) {
    fields.push(LogField {
        name: name.to_string(),
        field: Some(Field::UintField(val)),
    });
}

pub fn add_field_bool(fields: &mut Vec<LogField>, name: &str, val: bool) {
    fields.push(LogField {
        name: name.to_string(),
        field: Some(Field::BoolField(val)),
    });
}

impl fmt::Display for Level {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl fmt::Display for LogField {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "{}: {}",
            self.name,
            self.field.as_ref().map(|f| f.to_string()).unwrap_or_default()
        )
    }
}

impl fmt::Display for Field {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Field::StrField(s) => write!(f, "{s}"),
            Field::FloatField(fl) => write!(f, "{fl}"),
            Field::IntField(i) => write!(f, "{i}"),
            Field::UintField(u) => write!(f, "{u}"),
            Field::BoolField(b) => write!(f, "{b}"),
            Field::BinaryField(b) => write!(f, "{b:?}"),
            Field::ByteStringField(b) => write!(f, "{b:?}"),
            Field::TimeField(t) => write!(f, "{t:?}"),
            Field::DurationField(d) => write!(f, "{d:?}"),
        }
    }
}
