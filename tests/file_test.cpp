#include "zedio/async.hpp"
#include"zedio/fs/file.hpp"

using namespace zedio::async;
using namespace zedio::fs;

auto test() -> Task<void> {
    auto ret = co_await File::create("123.txt");
    if (!ret) {
        LOG_ERROR("{}", ret.error().message());
    }
    co_return;
}

auto main() -> int {
    auto runtime = Runtime::options().set_num_worker(1).build();
    runtime.block_on(test());
    return 0;
}