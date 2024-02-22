use monoio::io::{AsyncReadRent, AsyncWriteRentExt};
use monoio::net::{TcpListener, TcpStream};

const RESPONSE: &str = r"
HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
Content-Length: 13

Hello, World!";


#[monoio::main(worker_threads = 4)]
// #[monoio::main]
async fn main() {
    let listener = TcpListener::bind("192.168.15.33:6666").unwrap();
    println!("listening");
    loop {
        let (stream, _) = listener.accept().await.unwrap();
        monoio::spawn(process(stream));
    }
}


async fn process(mut stream: TcpStream) -> std::io::Result<()> {
    let mut buf: Vec<u8> = Vec::with_capacity(1024);
    let mut res;
    loop {
        // read
        (res, buf) = stream.read(buf).await;
        if res? == 0 {
            return Ok(());
        }

        (_, _) = stream.write_all(RESPONSE).await;
    }
}
