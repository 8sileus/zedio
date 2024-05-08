#include "zedio/core.hpp"
#include "zedio/fs/dir.hpp"
#include "zedio/fs/file.hpp"
#include "zedio/log.hpp"

using namespace zedio::async;
using namespace zedio::fs;
using namespace zedio;
using namespace zedio::log;

auto test_file_create() -> Task<void> {
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
    ret.value().seek(0, SEEK_SET);
    {
        std::string buf = "hahah\n";
        co_await ret.value().read_to_end(buf);
        console.info("{} {}", buf, buf.size());
    }
    if (auto fsync_ret = co_await ret.value().fsync_data(); !fsync_ret) {
        console.error("{} {}", fsync_ret.error().value(), fsync_ret.error().message());
    }
    co_return;
}

auto test_file_open() -> Task<void> {
    auto ret = co_await File::open("123.txt");
    if (!ret) {
        LOG_ERROR("{}", ret.error().message());
    }
    if (auto meta = co_await ret.value().metadata(); meta) {
        console.info("{}", meta.value().stx_size);
    } else {
        console.error("{} {}", meta.error().value(), meta.error().message());
    }
    std::string buf = "nono\n";
    co_await ret.value().read_to_end(buf);
    console.info("{}", buf);
    co_return;
}

auto test_read() -> Task<void> {
    auto ret = co_await fs::read_to_end<std::string>("123.txt");
    if (!ret) {
        LOG_ERROR("{}", ret.error().message());
    } else {
        LOG_INFO("test read {}", ret.value().data());
    }
}

auto test_rename() -> Task<void> {
    auto ret = co_await fs::rename("123.txt", "321.txt");
    if (!ret) {
        LOG_ERROR("rename failed {}", ret.error().message());
    } else {
        auto ret = co_await fs::File::open("123.txt");
        if (ret) {
            LOG_ERROR("rename failed");
        }
    }
}

auto test_mkdir() -> Task<void> {
    {
        auto ret = co_await fs::create_dir("test_dir");
        if (!ret) {
            LOG_ERROR("{}", ret.error().message());
        }
    }
    {
        auto ret = co_await fs::remove_dir("test_dir");
        if (!ret) {
            LOG_ERROR("{}", ret.error().message());
        }
    }
}

auto test_remove() -> Task<void> {
    {
        auto ret = co_await fs::File::create("test_remove.txt");
        if (!ret) {
            LOG_ERROR("{}", ret.error().message());
        }
    }
    {
        auto ret = co_await fs::remove_file("test_remove.txt");
        if (!ret) {
            LOG_ERROR("{}", ret.error().message());
        }
    }
}

auto test() -> Task<void> {
    co_await test_file_create();
    co_await test_file_open();
    co_await test_read();
    co_await test_rename();
    co_await test_mkdir();
    co_await test_remove();
}

auto main() -> int {
    zedio::runtime::MultiThreadBuilder::default_create().block_on(test());
    return 0;
}