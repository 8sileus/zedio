#include "zedio/core.hpp"
#include "zedio/io/stdio.hpp"
#include "zedio/log.hpp"

#include <string_view>
#include <vector>

using namespace zedio::async;
using namespace zedio::log;
using namespace zedio;

auto test() -> Task<void> {
    for (auto i = 0; i < 3; i++) {
        std::vector<char> buf(64);

        auto res = co_await io::input(buf);
        if (!res) {
            console.info("EOF");
            co_return;
        }
        console.info("read from stdin: {}", std::string_view{buf.data(), buf.size()});
        co_await io::print("████████████████████\n");
        co_await io::eprint("▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓\n");
    }
    co_return;
}

auto main() -> int {
    SET_LOG_LEVEL(LogLevel::Debug);
    auto runtime = Runtime::options().scheduler().set_num_workers(1).build();
    runtime.block_on(test());
}
