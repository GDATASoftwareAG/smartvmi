use crate::grpc_server::{GRPCServer, Stream};
use crate::pkg::vmi::v1::vmi_service_server::VmiService;
use crate::pkg::vmi::v1::{
    DumpMsgToFileRequest, DumpMsgToFileResponse, ListenForEventsRequest, ListenForEventsResponse,
};
use async_std::channel::Receiver;
use async_stream::try_stream;
use tonic::{Request, Response, Status, Streaming};

#[derive(Debug)]
pub struct GRPCVmiService {
    file_receiver: Receiver<DumpMsgToFileResponse>,
    event_receiver: Receiver<ListenForEventsResponse>,
}

impl GRPCVmiService {
    pub fn new(
        file_receiver: Receiver<DumpMsgToFileResponse>,
        event_receiver: Receiver<ListenForEventsResponse>,
    ) -> GRPCVmiService {
        GRPCVmiService {
            file_receiver,
            event_receiver,
        }
    }
}

#[tonic::async_trait]
impl VmiService for GRPCVmiService {
    type DumpMsgToFileStream = Stream<DumpMsgToFileResponse>;
    type ListenForEventsStream = Stream<ListenForEventsResponse>;

    async fn dump_msg_to_file(
        &self,
        request: Request<Streaming<DumpMsgToFileRequest>>,
    ) -> Result<Response<Self::DumpMsgToFileStream>, Status> {
        let mut grpc_stream = request.into_inner();

        let message_receiver = self.file_receiver.clone();

        let stream = try_stream! {
             loop {
                tokio::select! {
                    result = message_receiver.recv() => match result {
                         Ok(msg) => yield msg,
                         Err(_) => return,
                     },
                     _ = GRPCServer::wait_for_termination(&mut grpc_stream) => return,
                };
            }
        };

        Ok(Response::new(Box::pin(stream) as Self::DumpMsgToFileStream))
    }

    async fn listen_for_events(
        &self,
        request: Request<Streaming<ListenForEventsRequest>>,
    ) -> Result<Response<Self::ListenForEventsStream>, Status> {
        let mut grpc_stream = request.into_inner();

        let message_receiver = self.event_receiver.clone();

        let stream = try_stream! {
             loop {
                tokio::select! {
                    result = message_receiver.recv() => match result {
                         Ok(msg) => yield msg,
                         Err(_) => return,
                     },
                     _ = GRPCServer::wait_for_termination(&mut grpc_stream) => return,
                };
            }
        };

        Ok(Response::new(Box::pin(stream) as Self::ListenForEventsStream))
    }
}
