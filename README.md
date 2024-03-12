# Zedio

[![C++23](https://img.shields.io/static/v1?label=standard&message=C%2B%2B23&color=blue&logo=c%2B%2B&&logoColor=white&style=flat)](https://en.cppreference.com/w/cpp/compiler_support)
![Platform](https://img.shields.io/static/v1?label=platform&message=linux&color=dimgray&style=flat)

```
  ______  ______   _____    _____    ____  
 |___  / |  ____| |  __ \  |_   _|  / __ \ 
    / /  | |__    | |  | |   | |   | |  | |
   / /   |  __|   | |  | |   | |   | |  | |
  / /__  | |____  | |__| |  _| |_  | |__| |
 /_____| |______| |_____/  |_____|  \____/ 
                                                                       
```

Documentation: https://8sileus.github.io/zedio/

Zedio is an event-driven header library for writing asynchronous applications in modern C++:

## Feature:
+ Multithreaded, work-stealing based task scheduler. (reference [tokio](https://tokio.rs/))
+ Proactor event handling backed by [io_uring](https://github.com/axboe/liburing).
+ Zero overhead abstraction, no virtual, no dynamic

## Sub library: 
+ **I/O**
+ **NetWorking** 
+ **FileSystem** 
+ **Time** 
+ **Sync** 
+ **Log** 

It's being developed, if you're interested in zedio and want to participate in its development, see [contributing](./docs/zedio/contributing.md)

# Example
```C++
// An echo server
// Ignore all errors
#include "zedio/core.hpp"
#include "zedio/net.hpp"

using namespace zedio;
using namespace zedio::async;
using namespace zedio::net;

auto process(TcpStream stream) -> Task<void> {
    char buf[1024]{};
    while (true) {
        auto len = (co_await (stream.read(buf))).value();
        if (len == 0) {
            break;
        }
        co_await stream.write_all({buf, len});
    }
}

auto server() -> Task<void> {
    auto addr = SocketAddr::parse("localhost", 9999).value();
    auto listener = TcpListener::bind(addr).value();
    while (true) {
        auto [stream, addr] = (co_await listener.accept()).value();
        spawn(process(std::move(stream)));
    }
}

auto main() -> int {
    Runtime::create().block_on(server());
}
```