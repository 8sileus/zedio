# ZEDIO
Zedio is a runtime for writing asycnhronous applications with the C++.   
It's being developed, if you're interested in zedio and want to participate in its development, look [Development](#development)
```
  ______  ______   _____    _____    ____  
 |___  / |  ____| |  __ \  |_   _|  / __ \ 
    / /  | |__    | |  | |   | |   | |  | |
   / /   |  __|   | |  | |   | |   | |  | |
  / /__  | |____  | |__| |  _| |_  | |__| |
 /_____| |______| |_____/  |_____|  \____/ 
                                                                       
```

## Overview
Zedio is a event-driven platform for writing asynchronous applications with the C++, At a high level, it provides a few major components:
- A multithreaded, work-stealing based task scheduler. (reference tokio)
- A proactor backed by io_uring.
- Asynchronous TCP and UDP sockets.

## Install && Compile Tests
### Compiler requirements 
Your compiler must support C++23. My compiler is GCC 13.1.0.

Zedio is header only,and you can copy zedio into your project,or install it via CMake.
1. clone zedio
```
git clone https://github.com/8sileus/zedio
cmake -B build
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
#include "zedio/async.hpp"
#include "zedio/net.hpp"

using namespace zedio::async;
using namespace zedio::net;

auto process(TcpStream stream) -> Task<void> {
    char buf[1024];
    while (true) {
        auto len = (co_await stream.read(buf)).value();
        if (len == 0) {
            break;
        }
        co_await stream.write_all({buf, len});
    }
}

auto server() -> Task<void> {
    auto addr = SocketAddr::parse("localhost", 9898).value();
    auto listener = TcpListener::bind(addr).value();
    while (true) {
        auto [stream, peer_addr] = (co_await listener.accept()).value();
        spawn(process(std::move(stream)));
    }
}

auto main() -> int {
    auto runtime = Runtime::create();
    runtime.block_on(server());
    return 0;
}
```

## Dependencies
boost: https://github.com/boostorg/boost  
liburing: https://github.com/axboe/liburing

## Benchmark
[benchamrk](./docs/benchmark.md)

## Development
You can help us develop this project if you're familiar with C++20 standard and io_uring, we have lots of things that you can help. about:   
- Writing tests
- Writing benchmark
- Writing documentation
- Report bugs
- Share ideas
- Technical support
- Suggest someone

## Discussion
- QQ Group: 590815858
- Email: 1926004708@qq.com
