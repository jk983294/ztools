#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <unordered_set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

using std::string;

namespace zerg {
template <typename Enumeration>
auto as_integer(Enumeration const value) -> typename std::underlying_type<Enumeration>::type {
    return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

template <typename T>
inline void GetParamFromStr(const string& str, T& out) {
    out = T();
}
template <>
inline void GetParamFromStr<int32_t>(const string& str, int32_t& out) {
    out = std::stoi(str);
}
template <>
inline void GetParamFromStr<float>(const string& str, float& out) {
    out = std::stof(str);
}
template <>
inline void GetParamFromStr<double>(const string& str, double& out) {
    out = std::stod(str);
}
template <>
inline void GetParamFromStr<bool>(const string& str, bool& out) {
    out = (str == "true" || str == "1" || str == "True");
}

template <typename T>
inline T GetParamFromStrByType(const string& str) {
    T out;
    GetParamFromStr<T>(str, out);
    return out;
}
inline int32_t GetInt32FromStr(const string& str) { return GetParamFromStrByType<int32_t>(str); }
inline float GetFloatFromStr(const string& str) { return GetParamFromStrByType<float>(str); }
inline double GetDoubleFromStr(const string& str) { return GetParamFromStrByType<double>(str); }
inline bool GetBoolFromStr(const string& str) { return GetParamFromStrByType<bool>(str); }

template <typename T>
inline string to_string(const T& v) {
    return std::to_string(v);
}

template <>
inline string to_string(const std::string& v) {
    return v;
}

template <typename TContainer>
std::string head(const TContainer& container, size_t n) {
    std::string ret;
    if (n == 0)
        n = container.size();
    else
        n = std::min(n, container.size());
    for (size_t i = 0; i < n; ++i) {
        ret.append(to_string(container[i])).append(",");
    }
    return ret;
}

template <typename T>
bool is_identical(const std::vector<T>& a, const std::vector<T>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

/**
 * check a contains sub_a
 */
template <typename T>
bool is_subset(std::vector<T> a, std::vector<T> sub_a) {
    std::sort(a.begin(), a.end());
    std::sort(sub_a.begin(), sub_a.end());
    return std::includes(a.begin(), a.end(), sub_a.begin(), sub_a.end());
}

template <typename T>
bool has_common_element(std::vector<T> a, std::vector<T> b) {
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    std::vector<T> v_intersection;

    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(v_intersection));
    return !v_intersection.empty();
}

template <typename TKey, typename TValue>
std::vector<TKey> get_all_keys(const std::map<TKey, TValue>& m) {
    std::vector<TKey> ret;
    for (auto const& imap : m) ret.push_back(imap.first);
    return ret;
}

template <typename TKey, typename TValue>
std::vector<TKey> get_all_keys(const std::unordered_map<TKey, TValue>& m) {
    std::vector<TKey> ret;
    for (auto const& imap : m) ret.push_back(imap.first);
    return ret;
}

template <typename TKey, typename TValue>
std::vector<TValue> get_all_values(const std::map<TKey, TValue>& m) {
    std::vector<TValue> ret;
    for (auto const& imap : m) ret.push_back(imap.second);
    return ret;
}

template <typename TKey, typename TValue>
std::vector<TValue> get_all_values(const std::unordered_map<TKey, TValue>& m) {
    std::vector<TValue> ret;
    for (auto const& imap : m) ret.push_back(imap.second);
    return ret;
}

template <typename T>
std::vector<T> unique_stable(const std::vector<T>& vec) {
    std::vector<T> ret;
    std::unordered_set<T> s;
    for (auto& v : vec) {
        if (s.count(v) > 0) continue;
        ret.push_back(v);
        s.insert(v);
    }
    return ret;
}

template <typename T>
void vec_append(std::vector<T>& vec, const std::vector<T>& vec1) {
    vec.insert(vec.end(), vec1.begin(), vec1.end());
}

template <typename T, typename T1>
void init_container(std::vector<T>& c, size_t s, T1 init_value) {
    c.resize(s);
    std::fill(c.begin(), c.end(), init_value);
}

}

