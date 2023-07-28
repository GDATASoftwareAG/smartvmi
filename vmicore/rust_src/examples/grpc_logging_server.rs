use rust_grpc_server::bridge::ffi::Level;
use rust_grpc_server::grpc_server::new_server;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let addr = "/tmp/vmi.sock";
    let name = "FancyLogger";

    let srv = new_server(addr, false);
    let logger = srv.new_named_logger(name);
    let srv_handle = std::sync::Arc::new(srv);

    println!("Starting server...");
    let srv_handle_clone = srv_handle.clone();
    let join_handle = std::thread::spawn(move || {
        srv_handle_clone.start_server().unwrap();
        println!("Server start terminated");
    });

    println!("Writing log line");
    logger.log(Level::INFO, "Hello World", Vec::default())?;

    let (trigger, listener) = triggered::trigger();
    ctrlc::set_handler(move || trigger.trigger()).unwrap();

    println!("Press Ctrl-C to terminate...");
    listener.wait();
    println!("Stopping server...");
    srv_handle.stop_server(2000);
    println!("Server stopped");
    join_handle.join().expect("Failed to join thread");

    Ok(())
}
