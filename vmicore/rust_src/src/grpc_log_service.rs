use crate::bridge::ffi::Level;
use crate::grpc_server::{GRPCServer, Stream};
use crate::pkg::logging::api::v1::{LogLevel, LogMessage, ReadLogRequest, ReadLogResponse};
use crate::pkg::logging::service::v1::log_service_server::LogService;
use async_std::channel::Receiver;
use async_stream::try_stream;
use tonic::{Request, Response, Status, Streaming};

impl From<Level> for LogLevel {
    fn from(level: Level) -> Self {
        match level {
            Level::INFO => LogLevel::Info,
            Level::DEBUG => LogLevel::Debug,
            Level::WARN => LogLevel::Warn,
            Level::ERROR => LogLevel::Error,
            _ => LogLevel::Unknown,
        }
    }
}

#[derive(Debug)]
pub struct GRPCLogService {
    receiver: Receiver<LogMessage>,
}

impl GRPCLogService {
    pub fn new(receiver: Receiver<LogMessage>) -> GRPCLogService {
        GRPCLogService { receiver }
    }
}

#[tonic::async_trait]
impl LogService for GRPCLogService {
    type ReadLogsStream = Stream<ReadLogResponse>;

    async fn read_logs(
        &self,
        request: Request<Streaming<ReadLogRequest>>,
    ) -> Result<Response<Self::ReadLogsStream>, Status> {
        let mut grpc_stream = request.into_inner();

        let message_receiver = self.receiver.clone();

        let stream = try_stream! {
             loop {
                tokio::select! {
                    result = message_receiver.recv() => match result {
                            Ok(msg) => yield ReadLogResponse {
                                log: Some(msg),
                                trace_id: Vec::new(),
                                span_id: Vec::new()
                            },
                            Err(_) => return,
                        },
                    _ = GRPCServer::wait_for_termination(&mut grpc_stream) => return,
                };
            }
        };

        Ok(Response::new(Box::pin(stream) as Self::ReadLogsStream))
    }
}
