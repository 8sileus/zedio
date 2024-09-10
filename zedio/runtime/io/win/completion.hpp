#pragma once

// Linux
#include <liburing.h>

namespace zedio::io::detail {

struct IOData;

struct IOCompletion {
    int get_result() {
        return cqe->res;
    }

    IOData *get_data() {
        return reinterpret_cast<IOData *>(cqe->user_data);
    }

    io_uring_cqe *cqe;
};

} // namespace zedio::io::detail
