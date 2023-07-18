#pragma once

#include <format>
#include <iostream>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include <yaml-cpp/yaml.h>
#include <boost/lexical_cast.hpp>

#include "util/noncopyable.hpp"
#include "util/singleton.hpp"

namespace zed::config::detail {

class ConfigVariableBase : util::Noncopyable {
public:
    virtual ~ConfigVariableBase() = default;

    virtual void from_string(const std::string &str) = 0;
};

class ConfigUpdater : util::Noncopyable {
public:
    void listen(ConfigVariableBase *&var) {
        std::lock_guard lock(m_rw_mutex);
        m_variables.emplace(var);
    }

    void erase(ConfigVariableBase *&var) {
        std::lock_guard lock(m_rw_mutex);
        m_variables.erase(var);
    }

    void set_value(const std::string &value) {
        {
            std::lock_guard lock(m_rw_mutex);
            m_value = value;
        }
        std::shared_lock lock(m_rw_mutex);
        for (auto &var : m_variables) {
            var->from_string(m_value);
        }
    }

    auto has_value() -> bool {
        std::shared_lock lock(m_rw_mutex);
        return m_value != "";
    }

    auto value() -> const std::string & {
        std::shared_lock lock(m_rw_mutex);
        return m_value;
    }

private:
    std::string                              m_value{};
    std::shared_mutex                        m_rw_mutex{};
    std::unordered_set<ConfigVariableBase *> m_variables{};
};

template <typename T>
class ConfigVariable : ConfigVariableBase {
private:
    explicit ConfigVariable(T &&value) : m_value{std::forward<T>(value)} {}

    ~ConfigVariable() {
        auto ptr = m_updater.lock();
        if (ptr != nullptr) {
            ptr->erase(this);
        }
    }

    void from_string(const std::string &str) override {
        set_value(boost::lexical_cast<T, std::string>(str));
    }

    void set_value(const T &value) {
        {
            std::shared_lock lock(m_rw_mutex);
            if (m_value == value) {
                return;
            }
        }
        std::lock_guard lock(m_rw_mutex);
        m_value = value;
    }

    void set_updater(std::weak_ptr<ConfigUpdater> updater) { m_updater = updater; }

    auto value() -> const T & {
        std::shared_lock lock(m_rw_mutex);
        return m_value;
    }

public:
    std::shared_mutex            m_rw_mutex{};
    T                            m_value;
    std::weak_ptr<ConfigUpdater> m_updater;
};

class Configurator : util::Noncopyable {
public:
    Configurator() { load("zed.yaml"); }

    void load(const std::string &file_name) {
        std::list<std::pair<std::string, const YAML::Node>> nodes{};
        YAML::Node                                          root{YAML::LoadFile(file_name)};
        LoadYamlNodes("", root, nodes);
        std::lock_guard lock(m_mutex);
        for (auto &[key, node] : nodes) {
            if (key.empty()) {
                continue;
            }
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            if (m_updaters.find(key) == m_updaters.end()) {
                m_updaters.emplace(key, std::make_shared<ConfigUpdater>());
            }
            auto &updater = m_updaters[key];
            if (node.IsScalar()) {
                updater->set_value(node.Scalar());
            } else {
                std::stringstream ss;
                ss << node;
                updater->set_value(ss.str());
            }
        }
    }

    template <typename T>
    void update_variable(const std::string &name, ConfigVariable<T> &var) {
        std::lock_guard lock(m_mutex);
        if (m_updaters.find(name) == m_updaters.end()) {
            m_updaters.emplace(name, std::make_shared<ConfigUpdater>());
        }
        auto &updater = m_updaters[name];
        var.set_updater(updater);
        updater->listen(var);
        if (updater->has_value()) {
            var.from_string(updater->value());
        }
    }

    // for debug
    auto to_string() -> std::string {
        std::string     res = "";
        std::lock_guard lock(m_mutex);
        for (auto &[name, updater] : m_updaters) {
            res += std::format("{}:{}\n", name, updater->value());
        }
        return res;
    }

private:
    static void LoadYamlNodes(const std::string &prefix, const YAML::Node &node,
                              std::list<std::pair<std::string, const YAML::Node>> &output) {
        if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678")
            != std::string::npos) {
            std::cerr << std::format("config invalid name = {}", prefix);
            return;
        }
        if (node.IsMap()) {
            for (auto it = node.begin(); it != node.end(); ++it) {
                LoadYamlNodes(
                    prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(),
                    it->second, output);
            }
        } else {
            output.emplace_back(prefix, node);
        }
    }

private:
    std::mutex                                                      m_mutex;
    std::unordered_map<std::string, std::shared_ptr<ConfigUpdater>> m_updaters;
};

auto &g_configurator = util::Singleton<detail::Configurator>::Instance();

}  // namespace zed::config::detail

namespace zed::config {

void LoadConfigFromYaml(const std::string &file_name) { detail::g_configurator.load(file_name); }

template <typename T>
auto MakeVariable(const std::string &name, const T &default_value) -> detail::ConfigVariable<T> {
    detail::ConfigVariable<T> var(default_value);
    detail::g_configurator.update_variable(name, var);
    return var;
}

auto ToString() -> std::string { return detail::g_configurator.to_string(); }

}  // namespace zed::config