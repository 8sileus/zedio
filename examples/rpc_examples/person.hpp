#pragma once

#include <sstream>
#include <string>
#include <string_view>

class Person {
public:
    std::string name;
    int         age;

    [[nodiscard]]
    auto serialize() const -> std::string {
        std::ostringstream oss;
        oss << name << " " << age;
        return oss.str();
    }

    [[nodiscard]]
    static auto deserialize(std::string_view data) -> Person {
        std::istringstream iss({data.begin(), data.size()});
        std::string        name;
        int                age{};
        iss >> name >> age;
        return {name, age};
    }
};
