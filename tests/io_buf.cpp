#include "zedio/core.hpp"
#include "zedio/fs/file.hpp"
#include "zedio/io/buf_reader.hpp"
#include "zedio/io/buf_writer.hpp"
#include "zedio/log.hpp"

using namespace zedio::async;
using namespace zedio::fs;
using namespace zedio::log;
using namespace zedio;

auto create_file() -> Task<void> {
    auto ret = co_await File::options()
                   .read(true)
                   .write(true)
                   .create(true)
                   .truncate(true)
                   .permission(0666)
                   .open("read_line_test.txt");
    std::string n1 = "\n";
    std::string n2 = "\r\n";
    auto        writer_test_move = io::BufWriter(std::move(ret.value()));
    LOG_DEBUG("{}", writer_test_move.capacity());
    auto        writer = std::move(writer_test_move);
    for (int i = 0; i <= 10000; i += 1) {
        if (i & 1) {
            co_await writer.write_all(std::to_string(i) + n1);
        } else {
            co_await writer.write_all(std::to_string(i) + n2);
        }
    }
    co_await writer.flush();
    if (auto meta = co_await writer.writer().metadata(); meta) {
        LOG_INFO("{}", meta.value().stx_size);
    } else {
        LOG_ERROR("{} {}", meta.error().value(), meta.error().message());
    }
    if (auto fsync_ret = co_await writer.writer().fsync_data(); !fsync_ret) {
        LOG_ERROR("{} {}", fsync_ret.error().value(), fsync_ret.error().message());
    }
    LOG_INFO("create file succ");
    co_return;
}

auto read_line() -> Task<void> {
    auto ret = co_await File::open("read_line_test.txt");
    if (!ret) {
        LOG_ERROR("{}", ret.error().message());
    }
    if (auto meta = co_await ret.value().metadata(); meta) {
        LOG_INFO("{}", meta.value().stx_size);
    } else {
        LOG_ERROR("{} {}", meta.error().value(), meta.error().message());
    }
    auto        reader = io::BufReader(std::move(ret.value()));
    std::string line;
    while (true) {
        if (auto ret = co_await reader.read_line(line); !ret || ret.value() == 0) {
            break;
        } else {
            LOG_DEBUG("{}", line);
            line.clear();
        }
    }
    LOG_INFO("read line succ");
    co_return;
}

auto read_until() -> Task<void> {
    auto ret = co_await File::open("read_line_test.txt");
    if (!ret) {
        LOG_ERROR("{}", ret.error().message());
    }
    if (auto meta = co_await ret.value().metadata(); meta) {
        LOG_INFO("{}", meta.value().stx_size);
    } else {
        LOG_ERROR("{} {}", meta.error().value(), meta.error().message());
    }
    auto        reader = io::BufReader(std::move(ret.value()));
    std::string buf;
    while (true) {
        if (auto ret = co_await reader.read_until(buf, "\r\n"); !ret || ret.value() == 0) {
            break;
        } else {
            LOG_DEBUG("{}", buf);
            buf.clear();
        }
    }
    LOG_INFO("read until succ");
    co_return;
}

auto read_exact() -> Task<void> {
    auto ret = co_await File::open("read_line_test.txt");
    if (!ret) {
        LOG_ERROR("{}", ret.error().message());
    }
    if (auto meta = co_await ret.value().metadata(); meta) {
        LOG_INFO("{}", meta.value().stx_size);
    } else {
        LOG_ERROR("{} {}", meta.error().value(), meta.error().message());
    }
    auto reader = io::BufReader(std::move(ret.value()));
    auto buf = std::array<char, 10>{};
    while (true) {
        if (auto ret = co_await reader.read_exact(buf); !ret) {
            LOG_ERROR("{}", ret.error());
            break;
        } else {
            LOG_DEBUG("{}", buf.data());
        }
    }
    LOG_INFO("read exact succ");
    co_return;
}

auto read() -> Task<void> {
    auto ret = co_await File::open("read_line_test.txt");
    if (!ret) {
        LOG_ERROR("{}", ret.error().message());
    }
    if (auto meta = co_await ret.value().metadata(); meta) {
        LOG_INFO("{}", meta.value().stx_size);
    } else {
        LOG_ERROR("{} {}", meta.error().value(), meta.error().message());
    }
    auto reader_test_move = io::BufReader(std::move(ret.value()));
    LOG_DEBUG("{}", reader_test_move.capacity());
    auto reader = std::move(reader_test_move);
    auto buf = std::array<char, 10>{};
    while (true) {
        if (auto ret = co_await reader.read(buf); !ret || ret.value() == 0) {
            break;
        } else {
            LOG_DEBUG("{}", std::string_view{buf.data(), ret.value()});
        }
    }
    LOG_INFO("read succ");
    co_return;
}

auto test() -> Task<void> {
    co_await create_file();
    co_await read_line();
    co_await read_until();
    co_await read_exact();
    co_await read();
}

auto main() -> int {
    auto runtime = Runtime::options().set_num_workers(1).build();
    runtime.block_on(test());
    return 0;
}