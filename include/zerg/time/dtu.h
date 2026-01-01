#pragma once
#include <cstdint>
#include <vector>
#include <unordered_map>

namespace zerg {
constexpr uint64_t InValidDTUKey = std::numeric_limits<uint64_t>::max();

struct DTUMap {
    // Build index map from (date, ticktime, ukey) -> index
    void build_dtu_idx(const std::vector<int>& dates, const std::vector<int>& ticks, const std::vector<int>& ukeys, uint64_t base_offset);

    void build_dtu_idx(const std::vector<int>& dates, const std::vector<int>& ticks, const std::vector<int>& ukeys);

    uint64_t find_dtu_idx(int date, int tick, int ukey) const;

    std::unordered_map<int, std::unordered_map<int, std::unordered_map<int, size_t>>> m_index_map;
};
}