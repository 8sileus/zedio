#include "singleton.hpp"

#include <thread>
#include <unistd.h>

namespace zed::util {

pid_t getTid() noexcept {
    static thread_local pid_t t_tid = gettid();
    return t_tid;
}

}  // namespace zed::util
