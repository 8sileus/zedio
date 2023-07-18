#pragma once

#include "util/noncopyable.hpp"

namespace zed::util {

template <typename T>
class Singleton : Noncopyable {
public:
    static T& Instance() {
        static T instance;
        return instance;
    }
};

}  // namespace zed::util
