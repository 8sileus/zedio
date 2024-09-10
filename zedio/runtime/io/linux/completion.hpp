#pragma once

// Linux
#include <liburing.h>

namespace zedio::io::detail {
struct IOData;
} // namespace zedio::io::detail

namespace zedio::runtime::detail {

struct IOCompletion {
    int get_result() {
        return cqe->res;
    }

    io::detail::IOData *get_data() {
        return reinterpret_cast<io::detail::IOData *>(cqe->user_data);
    }

    io_uring_cqe *cqe;
};

} // namespace zedio::runtime::detail
