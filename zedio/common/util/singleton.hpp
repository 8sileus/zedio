#pragma once

namespace zedio::util {

template <typename T>
class Singleton {
    Singleton() = delete;
    ~Singleton() = delete;

public:
    [[nodiscard]]
    static auto instance() -> T & {
        static T instance;
        return instance;
    }
};

} // namespace zedio::util
