use std::error::Error;

use hyper_util::rt::TokioIo;
use tokio::{net::UnixStream, try_join};
use tonic::{
    transport::{Channel, Endpoint, Uri},
    Request,
};
use tower::service_fn;
use triggered::{Listener, Trigger};

use rust_grpc_server::pkg::logging::{
    api::v1::{LogLevel, ReadLogRequest},
    service::v1::log_service_client::LogServiceClient,
};
use rust_grpc_server::pkg::vmi::v1::{vmi_service_client::VmiServiceClient, ListenForEventsRequest};

const UNIX_SOCKET_ADDR: &str = "/tmp/vmi.sock";

async fn log_client(channel: Channel, trigger: Trigger, listener: Listener) -> Result<(), Box<dyn Error>> {
    let mut client = LogServiceClient::new(channel);

    let outbound = async_stream::stream! {
        // Request is currently ignored
        yield ReadLogRequest{level: LogLevel::Debug.into()};

        listener.await;
    };

    let response = client.read_logs(Request::new(outbound)).await?;
    let mut inbound = response.into_inner();

    while let Some(response) = inbound.message().await? {
        println!("{response:#?}");
    }

    trigger.trigger();

    Ok(())
}

async fn vmi_client(channel: Channel, trigger: Trigger, listener: Listener) -> Result<(), Box<dyn Error>> {
    let mut client = VmiServiceClient::new(channel);

    let outbound = async_stream::stream! {
        listener.await;

        yield ListenForEventsRequest{};
    };

    let response = client.listen_for_events(Request::new(outbound)).await?;
    let mut inbound = response.into_inner();

    while let Some(response) = inbound.message().await? {
        println!("{response:#?}");
    }

    trigger.trigger();

    Ok(())
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    // Url not used
    let channel = Endpoint::try_from("http://[::]:50051")?
        .connect_with_connector(service_fn(|_: Uri| async {
            // Connect to a Uds socket
            Ok::<_, std::io::Error>(TokioIo::new(UnixStream::connect(UNIX_SOCKET_ADDR).await?))
        }))
        .await?;

    let (trigger, listener) = triggered::trigger();
    let ctrlc_trigger = trigger.clone();
    ctrlc::set_handler(move || ctrlc_trigger.trigger()).unwrap();

    println!("Consuming logs as well as events. Press Ctrl-C to terminate...");

    try_join!(
        log_client(channel.clone(), trigger.clone(), listener.clone()),
        vmi_client(channel.clone(), trigger.clone(), listener.clone())
    )?;

    Ok(())
}
