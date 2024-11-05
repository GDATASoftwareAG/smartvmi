use crate::bridge::ffi::{Level, ProcessState};
use crate::grpc_log_service::GRPCLogService;
use crate::grpc_logger::GrpcLogger;
use crate::grpc_vmi_service::GRPCVmiService;
use crate::pkg::logging::api::v1::log_field::Field;
use crate::pkg::logging::api::v1::{LogField, LogMessage};
use crate::pkg::logging::service::v1::log_service_server::LogServiceServer;
use crate::pkg::vmi::v1::{
    listen_for_events_response::Message, vmi_service_server::VmiServiceServer, BsodDetected, DumpMsgToFileResponse,
    Event, ListenForEventsResponse, VmProcessEnd, VmProcessStart, VmiFinished, VmiReady,
};

use async_std::channel::{unbounded, Receiver, Sender};
use async_std::task;
use cxx::CxxVector;
use std::error::Error;
use std::fmt::Debug;
use std::pin::Pin;
use std::result;
use std::time::SystemTime;
use tokio::net::UnixListener;
use tokio_stream::wrappers::UnixListenerStream;
use tonic::transport::Server;
use tonic::{Status, Streaming};

pub type Stream<T> = Pin<Box<dyn futures_core::Stream<Item = result::Result<T, Status>> + Send + 'static>>;

pub struct AsyncQueue<T> {
    sender: Sender<T>,
    receiver: Receiver<T>,
}

pub struct GRPCServer {
    log_level: Level,
    enable_debug: bool,
    listen_addr: String,
    trigger: triggered::Trigger,
    listener: triggered::Listener,
    log_channel: AsyncQueue<LogMessage>,
    file_channel: AsyncQueue<DumpMsgToFileResponse>,
    event_channel: AsyncQueue<ListenForEventsResponse>,
}

pub fn new_server(listen_addr: &str, enable_debug: bool) -> Box<GRPCServer> {
    let (trigger, listener) = triggered::trigger();

    let (sender, receiver) = unbounded();
    let log_channel = AsyncQueue { sender, receiver };

    let (sender, receiver) = unbounded();
    let file_channel = AsyncQueue { sender, receiver };

    let (sender, receiver) = unbounded();
    let event_channel = AsyncQueue { sender, receiver };

    Box::new(GRPCServer {
        log_level: Level::INFO,
        enable_debug,
        listen_addr: listen_addr.to_string(),
        trigger,
        listener,
        log_channel,
        file_channel,
        event_channel,
    })
}

impl GRPCServer {
    #[tokio::main]
    pub async fn start_server(&self) -> Result<(), Box<dyn Error>> {
        let log_service = GRPCLogService::new(self.log_channel.receiver.clone());
        let vmi_service = GRPCVmiService::new(self.file_channel.receiver.clone(), self.event_channel.receiver.clone());

        let incoming = {
            let addr_ref = self.listen_addr.as_str();
            if std::path::Path::new(addr_ref).exists() {
                std::fs::remove_file(addr_ref).unwrap();
            }
            let uds = UnixListener::bind(addr_ref)?;

            UnixListenerStream::new(uds)
        };

        Server::builder()
            .add_service(LogServiceServer::new(log_service))
            .add_service(VmiServiceServer::new(vmi_service))
            .serve_with_incoming_shutdown(incoming, self.listener.clone())
            .await?;

        Ok(())
    }

    pub fn stop_server(&self, timeout_millis: u64) {
        self.log_channel.sender.close();
        self.file_channel.sender.close();
        self.event_channel.sender.close();

        let start_time = std::time::Instant::now();
        while (!self.file_channel.sender.is_empty()
            || !self.log_channel.sender.is_empty()
            || !self.event_channel.sender.is_empty())
            && start_time.elapsed().as_millis() < timeout_millis.into()
        {
            std::thread::sleep(core::time::Duration::from_millis(50));
        }
        self.trigger.trigger();
    }

    pub fn new_logger(&self) -> Box<GrpcLogger> {
        Box::new(GrpcLogger::new(
            self.log_level,
            self.enable_debug,
            self.log_channel.sender.clone(),
            Vec::new(),
        ))
    }

    pub fn new_named_logger(&self, name: &str) -> Box<GrpcLogger> {
        let log_field = LogField {
            name: "logger".to_string(),
            field: Some(Field::StrField(name.to_string())),
        };
        Box::new(GrpcLogger::new(
            self.log_level,
            self.enable_debug,
            self.log_channel.sender.clone(),
            vec![log_field],
        ))
    }

    pub fn set_log_level(&mut self, log_level: crate::bridge::ffi::Level) {
        self.log_level = log_level
    }

    pub fn write_message_to_file(&self, name: &str, message: &CxxVector<u8>) -> Result<(), Box<dyn Error>> {
        task::block_on(self.file_channel.sender.send(DumpMsgToFileResponse {
            filename: name.to_string(),
            message: message.iter().cloned().collect(),
        }))?;
        Ok(())
    }

    pub fn send_process_event(
        &self,
        process_state: ProcessState,
        process_name: &str,
        process_id: u32,
        cr3: &str,
    ) -> Result<(), Box<dyn Error>> {
        let (process_event, process_message) = match process_state {
            ProcessState::Started => (
                Event::VmProcessStart,
                Some(Message::VmProcessStart(VmProcessStart {
                    process_name: process_name.to_string(),
                    process_id,
                    cr3: cr3.to_string(),
                })),
            ),
            ProcessState::Terminated => (
                Event::VmProcessEnd,
                Some(Message::VmProcessEnd(VmProcessEnd {
                    process_name: process_name.to_string(),
                    process_id,
                    cr3: cr3.to_string(),
                })),
            ),
            _ => panic!("Unknown state"),
        };

        task::block_on(self.event_channel.sender.send(ListenForEventsResponse {
            event: process_event.into(),
            timestamp: Some(SystemTime::now().into()),
            message: process_message,
        }))?;
        Ok(())
    }

    pub fn send_bsod_event(self: &GRPCServer, code: i64) -> Result<(), Box<dyn Error>> {
        task::block_on(self.event_channel.sender.send(ListenForEventsResponse {
            event: Event::BsodDetected.into(),
            timestamp: Some(SystemTime::now().into()),
            message: Some(Message::BsodDetected(BsodDetected { code })),
        }))?;
        Ok(())
    }

    pub fn send_ready_event(self: &GRPCServer) -> Result<(), Box<dyn Error>> {
        task::block_on(self.event_channel.sender.send(ListenForEventsResponse {
            event: Event::VmiReady.into(),
            timestamp: Some(SystemTime::now().into()),
            message: Some(Message::Ready(VmiReady {})),
        }))?;
        Ok(())
    }

    pub fn send_termination_event(self: &GRPCServer) -> Result<(), Box<dyn Error>> {
        task::block_on(self.event_channel.sender.send(ListenForEventsResponse {
            event: Event::VmiFinished.into(),
            timestamp: Some(SystemTime::now().into()),
            message: Some(Message::Finished(VmiFinished {})),
        }))?;
        Ok(())
    }

    pub fn send_error_event(self: &GRPCServer, message: &str) -> Result<(), Box<dyn Error>> {
        task::block_on(self.event_channel.sender.send(ListenForEventsResponse {
            event: Event::Error.into(),
            timestamp: Some(SystemTime::now().into()),
            message: Some(Message::Error(crate::pkg::vmi::v1::Error {
                message: message.to_string(),
            })),
        }))?;
        Ok(())
    }

    pub fn send_in_mem_detection_event(self: &GRPCServer, message: &str) -> Result<(), Box<dyn Error>> {
        task::block_on(self.event_channel.sender.send(ListenForEventsResponse {
            event: Event::InMemDetection.into(),
            timestamp: Some(SystemTime::now().into()),
            message: Some(Message::InMemDetection(crate::pkg::vmi::v1::InMemDetection {
                detection: message.to_string(),
            })),
        }))?;
        Ok(())
    }

    pub async fn wait_for_termination<T: Debug>(stream: &mut Streaming<T>) {
        loop {
            match stream.message().await {
                Ok(msg) => match msg {
                    Some(request) => println!("Received client request: {request:#?}"),
                    None => {
                        println!("Connection closed");
                        break;
                    }
                },
                Err(e) => println!("Error in client channel: {e}"),
            }
        }
    }
}
