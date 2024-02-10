# ZEDIO 
Zedio is a runtime for writing asycnhronous applications with the C++. It is:
- **Fast**: You can trust the performance of C++.
- **Convenient**: It is used like golang.
- **Rust Style**: Rust is modern C++.
```
  ______  ______   _____    _____    ____  
 |___  / |  ____| |  __ \  |_   _|  / __ \ 
    / /  | |__    | |  | |   | |   | |  | |
   / /   |  __|   | |  | |   | |   | |  | |
  / /__  | |____  | |__| |  _| |_  | |__| |
 /_____| |______| |_____/  |_____|  \____/ 
                                                                       
```
## Overview
Zedio is a event-driven platform for writing asynchronous applications with C++, At a high level, it provides a few major components:
- A multithreaded, work-stealing based task scheduler.(copy from tokio)
- A proactor backed by linux systems's io_uring.
- Asynchronous TCP and UDP sockets.

## Install && Compile Tests
### Compiler requirements 
Your compiler must support C++23. My compiler is GCC 13.1.0.

Zedio is header only,and you can copy zedio into your project,or install it via CMake.
1. clone zedio
```
gitclone https://github.com/8sileus/zedio
cd zedio
mkdir build
cd build
```
2. install
```
cmake --install . # --prefix ./user_defined_install_path 
```
3. compile tests
```
cmake --build . # -j num_thread
ctest .
```

## Example
Writing an echo server using Zedio
``` C++
// Ignore all errors
#include "zed/async.hpp"
#include "zed/net.hpp"

using namespace zed::async;
using namespace zed::net;

auto process(TcpStream stream) -> Task<void> {
    char buf[1024];
    while (true) {
        auto len = (co_await stream.read(buf)).value();
        if (len == 0) {
            break;
        }
        co_await stream.write({buf, len});
    }
}

auto server() -> Task<void> {
    auto addr = SocketAddr::parse("localhost", 9898).value();
    auto listener = TcpListener::bind(addr).value();
    while (true) {
        auto [stream, peer_addr] = (co_await listener.accept()).value();
        spwan(process(std::move(stream)));
    }
}

auto main() -> int {
    auto runtime = Runtime::create();
    runtime.block_on(server());
    return 0;
}
```

## Performance comparison Tokio
OS：Ubuntu23.04  
Number of coro：4    
Memory：4G  
CPU：AMD Ryzen 5 3600 6-Core Processor  
command：./wrk -t4 -c1000 -d90s --latency http://192.168.15.33:7777/   
ZEDIO:  
![](./doc/png/zedio_benchmark.png)  
TOKIO:  
![](./doc/png/tokio_benchmark.png)  


## Dependencies
boost: https://github.com/boostorg/boost  
liburing: https://github.com/axboe/liburing

