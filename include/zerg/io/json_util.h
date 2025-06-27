#pragma once

#include <json.hpp>

namespace zerg {
template <typename T>
inline T json_get(const nlohmann::json& conf, const std::string& key, const T& default_val) {
    if (conf.contains(key)) {
        return conf[key].template get<T>();
    } else {
        return default_val;
    }
}

template <typename T>
inline T json_get(const nlohmann::json& conf, const std::string& key) {
    return conf[key].template get<T>();
}

template <typename T>
inline void json_get(const nlohmann::json& conf, const std::string& key, std::vector<T>& out) {
    if (conf.contains(key)) {
        auto& value = conf[key];
        if (value.is_array()) {
            for (auto& arr : value) {
                out.push_back(arr.template get<T>());
            }
        }
    }
}
}