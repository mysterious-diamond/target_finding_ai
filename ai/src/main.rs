use std::error::Error;

use tokio::net::windows::named_pipe::{NamedPipeServer, ServerOptions};

async fn await_pipe_connection() -> Result<NamedPipeServer, Box<dyn Error>> {
    let pipe: NamedPipeServer = ServerOptions::new().create(r"\\.\pipe\target-ai")?;

    println!("Waiting for connection from interface...");
    pipe.connect().await?;

    println!("Pipe connected succesfully. Awaiting input...");
    Ok(pipe)
}

#[tokio::main]
async fn main() {
    let pipe: NamedPipeServer = await_pipe_connection()
        .await
        .expect("Couldn't connect pipe...");

    let mut buffer = vec![0; 2 * 4];
    loop {
        pipe.readable()
            .await
            .expect("Error while waiting for message");

        match pipe.try_read(&mut buffer) {
            Ok(0) => break,
            Ok(n) => {
                println!("Received {} bytes", n);
                let _data: Vec<f32> = buffer[..n]
                    .chunks_exact(4)
                    .map(|chunk| {
                        let array: [u8; 4] = chunk.try_into().expect("Data broken or corrupted");
                        f32::from_le_bytes(array)
                    })
                    .collect();
            }
            Err(err) if err.kind() == std::io::ErrorKind::WouldBlock => continue,
            Err(err) if err.kind() == std::io::ErrorKind::BrokenPipe => break,
            Err(err) => {
                panic!(
                    "Error while trying to read message, error message \n{}",
                    err
                );
            }
        }
    }
}
