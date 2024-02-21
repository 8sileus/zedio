#pragma once

// copy from tokio

// C++
#include <random>
// C
#include <cinttypes>

namespace zedio {

class FastRand {
public:
    FastRand() {
        std::random_device dev;
        one_ = static_cast<uint32_t>(dev());
        two_ = static_cast<uint32_t>(dev());
    }

    auto fastrand_n(uint32_t n) -> uint32_t {
        return fastrand() % n;
        // auto result = static_cast<uint64_t>(fastrand()) * static_cast<uint64_t>(n);
        // return static_cast<uint32_t>(result >> 32);
    }

private:
    auto fastrand() -> uint32_t {
        auto s1 = one_;
        auto s0 = two_;
        s1 ^= s1 << 17;
        s1 = s1 ^ s0 ^ s1 >> 7 ^ s0 >> 16;
        one_ = s0;
        two_ = s1;
        return s0 + s1;
    }

private:
    uint32_t one_;
    uint32_t two_;
};

} // namespace zedio