#pragma once

namespace zedio::util {

template <typename T>
class Singleton {
public:
    Singleton() = delete;
    ~Singleton() = delete;

    [[nodiscard]]
    static auto instance() -> T & {
        static T instance;
        return instance;
    }
};

} // namespace zedio::util
