#pragma once

#include <chrono>
#include <format>
#include <iostream>
#include <thread>

namespace zed::util {

template <typename F, typename... Args>
void SpendTime(F&& f, Args&&... args) {
    auto start{std::chrono::steady_clock::now()};
    f(std::forward<Args>(args)...);
    auto end{std::chrono::steady_clock::now()};
    std::cout << std::format("total spend:{}\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - start));
}

}  // namespace zed::util
