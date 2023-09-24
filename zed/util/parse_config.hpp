#pragma once

// c++
#include <format>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

// depend
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>

namespace zed::util {

/// @brief thread safe parse config

enum class FileType {
    JSON,
    YAML,
    INI,
};

// template<FileType F>
class ParseConfig {
public:
    ParseConfig(const std::string &file_name) { load_from_file(file_name); }

    void load_from_file(const std::string &file_name) {
        std::lock_guard                                         lock(datas_mutex_);
        std::vector<std::pair<std::string, const YAML::Node &>> nodes{};
        YAML::Node                                              root{YAML::LoadFile(file_name)};
        this->load("", root, nodes);
        for (auto &[key, node] : nodes) {
            if (key.empty()) {
                continue;
            }
            std::transform(key.begin(), key.end(), key.begin(), tolower);
            if (node.IsScalar()) {
                datas_.emplace(key, node.Scalar());
            } else {
                std::stringstream ss;
                ss << node;
                datas_.emplace(key, ss.str());
            }
        }
    }

    template <typename T>
    auto get(const std::string &key, const T &default_value) -> T {
        std::shared_lock lock(datas_mutex_);
        if (datas_.count(key)) {
            return boost::lexical_cast<T>(datas_[key]);
        } else {
            return default_value;
        }
    }

private:
    static void load(const std::string &prefix, const YAML::Node &node,
                     std::vector<std::pair<std::string, const YAML::Node &>> &output) {
        if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != prefix.npos) {
            throw std::runtime_error(std::format("config file has invalid key", prefix));
        }
        if (node.IsMap()) {
            for (const auto &it : node) {
                load(prefix.empty() ? it.first.Scalar() : prefix + "." + it.first.Scalar(),
                     it.second, output);
            }
        } else {
            output.emplace_back(prefix, node);
        }
    }

private:
    std::shared_mutex                            datas_mutex_{};
    std::unordered_map<std::string, std::string> datas_{};
};

constexpr std::string ZED_CONFIG_FILE{"zed.yaml"};

} // namespace zed::util
