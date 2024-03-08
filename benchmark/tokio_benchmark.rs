use tokio::io::AsyncReadExt;
use tokio::io::AsyncWriteExt;
use tokio::net::{TcpListener, TcpStream};

const RESPONSE: &str = r"
HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
Content-Length: 13

Hello, World!";

// #[tokio::main(worker_threads = 4)]
#[tokio::main]
async fn main() {
    let listener = TcpListener::bind("192.168.15.33:7777").await.unwrap();
    loop {
        let (stream, _) = listener.accept().await.unwrap();
        tokio::spawn(async move {
            process(stream).await;
        });
    }
}

async fn process(mut stream: TcpStream) {
    let mut buf = [0; 128];
    loop {
        match stream.read(&mut buf).await {
            Ok(len) => {
                if len == 0 {
                    return;
                } else {
                    stream.write_all(RESPONSE.as_bytes()).await.unwrap()
                }
            }
            Err(err) => println!("error: {}", err),
        }
    }
}
