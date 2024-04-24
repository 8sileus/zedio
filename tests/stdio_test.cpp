#include "zedio/core.hpp"
#include "zedio/io/stdio.hpp"
#include "zedio/log.hpp"

#include <string_view>
#include <vector>

using namespace zedio::async;
using namespace zedio::log;
using namespace zedio;

auto test() -> Task<void> {
    auto &stdin = io::stdin();
    auto &stdout = io::stdout();
    auto &stderr = io::stderr();

    for (auto i = 0; i < 3; i++) {
        std::vector<char> buf(64);
        auto              res = co_await stdin.read(buf);
        if (!res) {
            console.info("EOF");
            co_return;
        }
        console.info("read from stdin: {}", std::string_view{buf.data(), buf.size()});
        co_await stdout.write("████████████████████\n");
        co_await stderr.write("▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓\n");
    }
    co_return;
}

auto main() -> int {
    SET_LOG_LEVEL(LogLevel::Debug);
    auto runtime = Runtime::options().scheduler().set_num_workers(1).build();
    runtime.block_on(test());
}
