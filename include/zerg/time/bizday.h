#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace flow {

class BizDayConfig {
public:
    BizDayConfig() {}
    BizDayConfig(const std::string& bizDayFile) { init(bizDayFile); }
    void init(const std::string& bizDayFile);

public:
    bool checkBizDay(int32_t date) const;
    std::vector<int32_t> bizDayRange(int32_t startDate, int32_t endDate) const;
    int32_t lowerBound(int32_t date) const;
    int lowerBoundIndex(int32_t date) const;
    int32_t next(int32_t date) const;
    int32_t prev(int32_t date) const;
    int32_t offset(int32_t date, int offset) const;
    int32_t first_day() const;
    int32_t last_day() const;

private:
    std::unordered_map<int32_t, size_t> bizDayIndexMap_;
    std::vector<int32_t> bizDaySorted_;
};

}
