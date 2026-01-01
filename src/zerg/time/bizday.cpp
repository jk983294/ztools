#include <zerg/time/bizday.h>
#include <zerg/string.h>
#include <iostream>
#include <numeric>
#include <fstream>
#include <algorithm>

namespace zerg {

void BizDayConfig::init(const std::string& bizDayFile) {
    std::ifstream fs(bizDayFile);
    if (!fs.is_open()) {
        throw std::runtime_error("Failed to open bizday file: ");
    }

    std::string line;
    std::getline(fs, line);
    if (!line.empty() && !std::isdigit(line.front())) {
        std::cout << "skip calendar header " << line << std::endl;
    } else {
        zerg::Trim(line);
        int32_t date = std::stoi(line);
        bizDaySorted_.push_back(date);
    }

    while (std::getline(fs, line)) {
        zerg::Trim(line);
        int32_t date = std::stoi(line);
        bizDaySorted_.push_back(date);
    }

    if (bizDaySorted_.empty()) {
        throw std::runtime_error("No biz dates loaded from file: ");
    }

    std::sort(bizDaySorted_.begin(), bizDaySorted_.end());

    for (size_t i = 0; i < bizDaySorted_.size(); ++ i) {
        bizDayIndexMap_[bizDaySorted_[i]] = i;
    }
}

bool BizDayConfig::checkBizDay(int32_t date) const {
    auto it = bizDayIndexMap_.find(date);
    return it != bizDayIndexMap_.end();
}

std::vector<int32_t> BizDayConfig::bizDayRange(int32_t startDate, int32_t endDate) const {
    std::vector<int32_t> ret;
    int startIndex = lowerBoundIndex(startDate);
    if (startIndex < 0) {
        return ret;
    }
    for (size_t i = startIndex; i < bizDaySorted_.size(); ++ i) {
        int32_t date = bizDaySorted_[i];
        if (date > endDate) {
            break;
        }
        ret.push_back(date);
    }
    return ret;
}

int32_t BizDayConfig::lowerBound(int32_t date) const {
    if (date < bizDaySorted_.front() || date > bizDaySorted_.back()) {
        return std::numeric_limits<int32_t>::max();
    }

    auto idx = std::lower_bound(bizDaySorted_.begin(), bizDaySorted_.end(), date);
    if (idx != bizDaySorted_.end()) {
        return *idx;
    }

    return std::numeric_limits<int32_t>::max();
}

int BizDayConfig::lowerBoundIndex(int32_t date) const {
    if (date < bizDaySorted_.front() || date > bizDaySorted_.back()) {
        return -1;
    }

    auto idx = std::lower_bound(bizDaySorted_.begin(), bizDaySorted_.end(), date);
    if (idx != bizDaySorted_.end()) {
        return idx - bizDaySorted_.begin();
    }

    return -1;
}

int32_t BizDayConfig::next(int32_t date) const {
    auto ub = std::upper_bound(bizDaySorted_.begin(), bizDaySorted_.end(), date);
    return *ub;
}

int32_t BizDayConfig::prev(int32_t date) const {
    auto lb = std::lower_bound(bizDaySorted_.begin(), bizDaySorted_.end(), date);
    return *(lb - 1);
}

int32_t BizDayConfig::offset(int32_t date, int offset) const {
    auto lb = std::lower_bound(bizDaySorted_.begin(), bizDaySorted_.end(), date);
    int idx = lb - bizDaySorted_.begin();
    idx += offset;
    if (idx <= 0) return bizDaySorted_.front();
    else if (idx >= (int)bizDaySorted_.size()) return bizDaySorted_.back();
    else return bizDaySorted_[idx];
}
int32_t BizDayConfig::first_day() const {
    return bizDaySorted_.front();
}
int32_t BizDayConfig::last_day() const {
    return bizDaySorted_.back();
}

std::vector<int32_t> BizDayConfig::prev_n(int32_t date, int n, bool include_date) const {
    std::vector<int32_t> ret;
    auto lb = std::lower_bound(bizDaySorted_.begin(), bizDaySorted_.end(), date);
    int idx = lb - bizDaySorted_.begin();
    
    // If the date is not exactly a business day, start from the previous business day
    if (lb == bizDaySorted_.end() || *lb != date) {
        idx--;
    } else {
        if (!include_date) {
            idx--;
        }
    }
    
    // Collect n business days
    for (int i = 0; i < n && idx >= 0; ++i) {
        ret.insert(ret.begin(), bizDaySorted_[idx]);
        idx--;
    }
    
    return ret;
}

}
