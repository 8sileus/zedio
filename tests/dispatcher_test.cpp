#include "async.hpp"
#include "async/detail/dispatcher.hpp"
#include "log.hpp"
// C++
#include <string>

using namespace zed::async;

int p[2];

auto r() -> Task<void> {
    while (true) {
        char buf[1024]{0};
        auto n = co_await zed::async::Read(p[0], buf, sizeof(buf), 0);
        zed::log::console.info("read {}", buf);
        co_await zed::async::Sleep(1s);
    }
}

auto w() -> Task<void> {
    long long num = 1;
    while (true) {
        auto str = std::to_string(num++);
        auto n = co_await zed::async::Write(p[1], str.c_str(), str.size(), 0);
        zed::log::console.info("write {}", str);
        co_await zed::async::Sleep(1s);
    }
}

int main() {
    pipe(p);
    std::cout << "thread num: " << std::thread::hardware_concurrency() << std::endl;
    detail::Dispatcher dis;
    dis.dispatch(r());
    dis.dispatch(w());
    std::this_thread::sleep_for(std::chrono::seconds(6));
    return 0;
}