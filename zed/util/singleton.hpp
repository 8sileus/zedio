#pragma once

namespace zed::util {

template <typename T>
class Singleton {
public:
    Singleton() = delete;
    ~Singleton() = delete;

    [[nodiscard]]
    static auto Instance() -> T & {
        static T instance;
        return instance;
    }
};

} // namespace zed::util
