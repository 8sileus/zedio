#include "util/config.hpp"

using namespace zed;

int main() {
    auto str = config::ToString();
    std::cout << str << "\n";
    return 0;
}