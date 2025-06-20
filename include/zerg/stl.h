#pragma once

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

template <typename T>
void pop_front_vector(std::vector<T>& vec) {
    assert(!vec.empty());
    vec.erase(vec.begin());
}

template <typename T>
bool HasItem(std::vector<T>& c, T val) {
    return (std::find(c.begin(), c.end(), val) != c.end());
}

template <typename K, typename V>
bool HasKey(const std::map<K, V>& c, const K& key) {
    return (c.find(key) != c.end());
}

template <typename K, typename V>
bool HasKey(const std::unordered_map<K, V>& c, const K& key) {
    return (c.find(key) != c.end());
}

template <typename K>
bool HasKey(const std::set<K>& c, const K& key) {
    return (c.find(key) != c.end());
}

template <typename K>
bool HasKey(const std::unordered_set<K>& c, const K& key) {
    return (c.find(key) != c.end());
}

template <typename Type>
inline std::ostream& operator<<(std::ostream& os, const std::set<Type>& _set) {
    for (auto itr = _set.begin(); itr != _set.end(); itr++) os << *itr << " ";
    return os;
}

template <typename Type>
inline std::ostream& operator<<(std::ostream& os, const std::vector<Type>& v) {
    for (auto itr = v.begin(); itr != v.end(); itr++) os << *itr << " ";
    return os;
}
template <typename Type>
inline std::ostream& operator<<(std::ostream& os, const std::vector<std::vector<Type> >& v) {
    for (auto itr = v.begin(); itr != v.end(); itr++) os << *itr << std::endl;
    return os;
}

template <typename T>
void reset_vector(std::vector<T> &vec, T init_val = T()) {
    std::fill(vec.begin(), vec.end(), init_val);
}

template <typename T>
void reset_vector(std::vector<std::vector<T>> &vec, T init_val = T()) {
    for (auto &sub_vec : vec) {
        std::fill(sub_vec.begin(), sub_vec.end(), init_val);
    }
}
