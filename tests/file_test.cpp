#include "zedio/async.hpp"
#include "zedio/fs/file.hpp"
#include "zedio/log.hpp"

using namespace zedio::async;
using namespace zedio::fs;
using namespace zedio::log;

auto create_file() -> Task<void> {
    auto ret = co_await File::options()
                   .read(true)
                   .write(true)
                   .create(true)
                   .truncate(true)
                   .permission(0666)
                   .open("123.txt");
    console.info("offset {}", lseek(ret.value().fd(), 0, SEEK_CUR));
    if (!ret) {
        LOG_ERROR("{}", ret.error().message());
    }
    for (int i = 0; i < 10; i += 1) {
        co_await ret.value().write("1234567910\n");
    }
    if (auto meta = co_await ret.value().metadata(); meta) {
        console.info("{}", meta.value().stx_size);
    } else {
        console.error("{} {}", meta.error().value(), meta.error().message());
    }
    console.info("offset {}", lseek(ret.value().fd(), 0, SEEK_SET));
    {
        std::string buf;
        co_await ret.value().read_to_string(buf);
        console.info("{} {}", buf, buf.size());
    }
    if (auto fsync_ret = co_await ret.value().fsync_data(); !fsync_ret) {
        console.error("{} {}", fsync_ret.error().value(), fsync_ret.error().message());
    }
    {
        std::string buf;
        co_await ret.value().read_to_string(buf);
        console.info("{} {}", buf, buf.size());
    }
    co_return;
}

auto open_file() -> Task<void> {
    auto ret = co_await File::open("123.txt");
    if (!ret) {
        LOG_ERROR("{}", ret.error().message());
    }
    if (auto meta = co_await ret.value().metadata(); meta) {
        console.info("{}", meta.value().stx_size);
    } else {
        console.error("{} {}", meta.error().value(), meta.error().message());
    }
    std::string buf;
    co_await ret.value().read_to_string(buf);
    console.info("{}", buf);
}

auto test() -> Task<void> {
    co_await create_file();
    co_await open_file();
}

auto main() -> int {
    auto runtime = Runtime::options().set_num_worker(1).build();
    runtime.block_on(test());
    return 0;
}