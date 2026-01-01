#include <zerg/time/dtu.h>
#include <zerg/log.h>


namespace zerg {
void DTUMap::build_dtu_idx(const std::vector<int>& dates, const std::vector<int>& ticks, const std::vector<int>& ukeys, uint64_t base_offset) {
    size_t curr_len = ukeys.size();
    for (size_t i = 0; i < curr_len; i++) {
        int date_val = dates[i];
        int tick_val = ticks[i];
        int ukey_val = ukeys[i];
        if (find_dtu_idx(date_val, tick_val, ukey_val) != InValidDTUKey) {
            ZLOG_THROW("dupe found for dtu=%d,%d,%d", date_val, tick_val, ukey_val);
        }
        m_index_map[date_val][tick_val][ukey_val] = base_offset + i;
    }
}

void DTUMap::build_dtu_idx(const std::vector<int>& dates, const std::vector<int>& ticks, const std::vector<int>& ukeys) {
    build_dtu_idx(dates, ticks, ukeys, 0UL);
}

uint64_t DTUMap::find_dtu_idx(int date, int tick, int ukey) const {
    auto it0 = m_index_map.find(date);
    if (it0 == m_index_map.end()) return InValidDTUKey;
    auto& t_u2i = it0->second;

    auto it1 = t_u2i.find(tick);
    if (it1 == t_u2i.end()) return InValidDTUKey;
    auto& u2i = it1->second;

    auto it2 = u2i.find(ukey);
    if (it2 == u2i.end()) return InValidDTUKey;
    return it2->second;
}

}