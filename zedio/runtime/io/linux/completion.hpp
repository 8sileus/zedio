#pragma once

// Linux
#include <liburing.h>

namespace zedio::io::detail {
struct IOData;
} // namespace zedio::io::detail

namespace zedio::runtime::detail {

struct IOCompletion {
    auto get_result(this IOCompletion &self) -> int {
        return self.cqe->res;
    }

    auto get_data(this IOCompletion &self) -> io::detail::IOData * {
        return reinterpret_cast<io::detail::IOData *>(self.cqe->user_data);
    }

    io_uring_cqe *cqe;
};

} // namespace zedio::runtime::detail
