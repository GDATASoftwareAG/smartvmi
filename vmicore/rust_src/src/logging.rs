use std::fmt;

use crate::{
    bridge::ffi::Level,
    hive_operations::logging::{log_field::Field, LogField},
};
use cxx::CxxString;

#[derive(thiserror::Error, Debug)]
pub enum LogError {
    #[error("log level not recognized")]
    LogLevelParseError,
}

pub fn convert_to_log_level(level: &CxxString) -> Result<Level, Box<dyn std::error::Error>> {
    match level.to_str() {
        Ok("debug") => Ok(Level::DEBUG),
        Ok("info") => Ok(Level::INFO),
        Ok("warning") => Ok(Level::WARN),
        Ok("error") => Ok(Level::ERROR),
        Err(e) => Err(Box::new(e)),
        _ => Err(Box::new(LogError::LogLevelParseError)),
    }
}

pub fn field_str(name: &CxxString, val: &CxxString) -> Box<LogField> {
    new_field(name.to_string(), Field::StrField(val.to_string()))
}

pub fn field_i64(name: &CxxString, val: i64) -> Box<LogField> {
    new_field(name.to_string(), Field::IntField(val))
}

pub fn field_float64(name: &CxxString, val: f64) -> Box<LogField> {
    new_field(name.to_string(), Field::FloatField(val))
}

pub fn field_uint64(name: &CxxString, val: u64) -> Box<LogField> {
    new_field(name.to_string(), Field::UintField(val))
}

pub fn field_bool(name: &CxxString, val: bool) -> Box<LogField> {
    new_field(name.to_string(), Field::BoolField(val))
}

fn new_field(name: String, field: Field) -> Box<LogField> {
    Box::new(LogField {
        name,
        field: Some(field),
    })
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
