#include <sys/time.h>
#include <cmath>
#include <cstring>
#include <ctime>
#include <fstream>
#include <vector>
#include <zerg/log.h>
#include <zerg/string.h>
#include <zerg/time/time.h>
#include <zerg/time/clock.h>

using std::string;

namespace zerg {
static std::vector<int32_t> _gen_trading_days(int year) {
    auto tl1 = GenAllDaysInYear(year - 1);
    auto tl2 = GenAllDaysInYear(year);
    auto tl3 = GenAllDaysInYear(year + 1);
    std::move(tl2.begin(), tl2.end(), std::back_inserter(tl1));
    std::move(tl3.begin(), tl3.end(), std::back_inserter(tl1));
    return tl1;
}
Clock::Clock(bool _isStrictTradingDay) {
    isStrictTradingDay = _isStrictTradingDay;
    tradingDayIndex = 0;
    clock_gettime(CLOCK_REALTIME, &core);
    SetTimeInfo();

    if (not isStrictTradingDay) {
        SetTradingDayInfo(_gen_trading_days(timeinfo.tm_year + 1900));
        SetFromCore();
    }
}

bool Clock::IsTradingDay() const {
    if (tradingDays.empty()) ZLOG_THROW("TradingDayList is empty, cannot decide if trading day.");
    return isTradingDay;
}

Clock::Clock(const std::string& filename) {
    tradingDayIndex = 0;
    clock_gettime(CLOCK_REALTIME, &core);
    SetTimeInfo();

    SetTradingDayInfo(filename);
    SetFromCore();
}

Clock::Clock(const std::vector<int32_t>& tl) {
    tradingDayIndex = 0;
    clock_gettime(CLOCK_REALTIME, &core);
    SetTimeInfo();

    SetTradingDayInfo(tl);
    SetFromCore();
}

Clock::Clock(const Clock& clock) {
    tradingDayIndex = 0;
    year = clock.year;
    month = clock.month;
    day = clock.day;
    hour = clock.hour;
    adjust_last_hour = clock.adjust_last_hour;
    min = clock.min;
    sec = clock.sec;
    nanoSecond = clock.nanoSecond;
    elapse = clock.elapse;
    core = clock.core;
    SetTimeInfo();

    tradingDays = clock.tradingDays;
    if (isStrictTradingDay) {
        if (tradingDays.empty()) ZLOG_THROW("Tradingday list cannot be empty for trading clock");
    }
    if (!tradingDays.empty()) SetTradingDayInfo(tradingDays);

    SetFromCore();
}

Clock& Clock::operator=(const Clock& clock) {
    if (this == &clock) {
        return *this;
    }
    year = clock.year;
    month = clock.month;
    day = clock.day;
    hour = clock.hour;
    adjust_last_hour = clock.adjust_last_hour;
    min = clock.min;
    sec = clock.sec;
    nanoSecond = clock.nanoSecond;
    elapse = clock.elapse;
    tradingDays = clock.tradingDays;
    core = clock.core;
    SetTimeInfo();
    if (isStrictTradingDay) {
        if (tradingDays.empty()) ZLOG_THROW("Trading day list cannot be empty for trading clock");
    }
    if (!tradingDays.empty()) SetTradingDayInfo(tradingDays);
    SetFromCore();
    return *this;
}

// use clock to represent a date or time
Clock::Clock(const char* string_time, const char* format) {
    if (isStrictTradingDay) {
        ZLOG_THROW("Trading Clock must be supplied with trading day list");
    }

    // first make timeinfo back to epoch
    const time_t temp = 0;
    auto temp_tm = localtime(&temp);
    timeinfo = *temp_tm;

    // parse it now
    if (strptime(string_time, format, &timeinfo) == nullptr) {
        ZLOG_THROW("Clock cannot properly parse your time string: %s with format: %s", string_time, format);
    }

    core.tv_sec = mktime(&timeinfo);
    core.tv_nsec = 0;
    SetFromCore();
    adjust_last_hour = timeinfo.tm_hour;
    tradingDay = 0;
    nextTradingDay = 0;
    prevTradingDay = 0;
    tradingDayIndex = 0;
    isTradingDay = false;
}

// use clock to represent a date or time
Clock::Clock(const char* string_time, const char* format, std::vector<int32_t>& list) {
    // first make timeinfo back to epoch
    const time_t temp = 0;
    auto temp_tm = localtime(&temp);
    timeinfo = *temp_tm;
    isStrictTradingDay = true;

    // parse it now
    if (strptime(string_time, format, &timeinfo) == nullptr)
        ZLOG_THROW("Clock cannot properly parse time string: %s with format: %s", string_time, format);
    core.tv_sec = mktime(&timeinfo);
    core.tv_nsec = 0;
    SetTimeInfo();
    adjust_last_hour = timeinfo.tm_hour;
    SetTradingDayInfo(list);
    SetFromCore();
}

// use clock to represent a date or time
Clock::Clock(const char* string_time, const char* format, const char* filename) {
    // first make timeinfo back to epoch
    const time_t temp = 0;
    auto temp_tm = localtime(&temp);
    timeinfo = *temp_tm;
    year = month = day = hour = min = sec = nanoSecond = 0;

    // parse it now
    if (strptime(string_time, format, &timeinfo) == nullptr) {
        ZLOG_THROW("Clock cannot properly parse time string: %s with format: %s", string_time, format);
    }

    core.tv_sec = mktime(&timeinfo);
    core.tv_nsec = 0;
    SetTimeInfo();
    adjust_last_hour = timeinfo.tm_hour;
    SetTradingDayInfo(filename);
    SetFromCore();
}

//! set trading day list
void Clock::SetTradingDayInfo(const std::vector<int32_t>& tl) {
    tradingDays = tl;
    auto todayInt = DateToIntReal();
    auto yesterdayInt = PrevDateToIntReal();
    if (tl.empty()) {
        ZLOG_THROW("Clock does not accept empty trading day list");
    }

    if (tradingDays.front() > todayInt) {
        ZLOG_THROW("TradingList not includes today");
    } else if (tradingDays.back() < todayInt) {
        ZLOG_THROW("TradingList not includes today");
    }

    for (size_t i = 0; i < tradingDays.size(); ++i) {
        if (tradingDays[i] >= todayInt) {
            tradingDayIndex = i;
            tradingDay = tradingDays[i];
            if (i < tradingDays.size() - 1) nextTradingDay = tradingDays[i + 1];
            if (i > 0) prevTradingDay = tradingDays[i - 1];

            // whether today is trading day
            isTradingDay = tradingDays[i] == todayInt;

            // whether yesterday is a trading day
            isPrevTradingDay = tradingDays[i - 1] == yesterdayInt;
            return;
        }
    }
}

// set trading day list
void Clock::SetTradingDayInfo(const std::string& filename) {
    std::vector<int32_t> tl;
    std::ifstream ifs;
    ifs.open(filename.c_str());
    if (!ifs.is_open()) {
        ZLOG_THROW("cannot open info file %s", filename.c_str());
    }

    // parse info file
    std::string line;
    getline(ifs, line);
    while (getline(ifs, line)) {
        if (line.empty() || !(line[0] >= '0' && line[0] <= '9')) continue;
        tl.push_back(std::stoi(line));
    }
    ifs.close();
    SetTradingDayInfo(tl);
}

// set clock from epoch time long
Clock& Clock::Set(time_t epoch_time) {
    memset(&core, 0, sizeof(timespec));
    core.tv_sec = epoch_time;
    auto temp_tm = localtime(&core.tv_sec);
    timeinfo = *temp_tm;
    SetFromCore();
    return *this;
}

Clock& Clock::Reset() {
    Set(0);
    return *this;
}

Clock& Clock::Update(const char* string_time, const char* format) {
    // first make timeinfo back to epoch
    time_t temp = 0;
    auto temp_tm = localtime(&temp);
    timeinfo = *temp_tm;
    year = month = day = hour = min = sec = 0;
    nanoSecond = 0;

    // parse it now
    if (strptime(string_time, format, &timeinfo) == nullptr) {
        ZLOG_THROW("Clock cannot properly parse time string: %s with format: %s", string_time, format);
    }

    core.tv_sec = mktime(&timeinfo);
    core.tv_nsec = 0;
    SetFromCore();
    return *this;
}

// update time and date to current timestamp
Clock& Clock::Update() {
    clock_gettime(CLOCK_REALTIME, &core);
    SetFromCore();
    return *this;
}

// just update time
Clock& Clock::UpdateTime() {
    clock_gettime(CLOCK_REALTIME, &core);
    SetTimeInfo();
    if (isStrictTradingDay) {
        Normalize_trading(DateToIntReal(), tradingDay, nextTradingDay, prevTradingDay, tradingDayIndex, isTradingDay,
                          isPrevTradingDay, core.tv_sec, tradingtimeinfo, tradingDays);
        FillTime(tradingtimeinfo, hour, min, sec);
    } else {
        FillTime(timeinfo, hour, min, sec);
    }
    nanoSecond = core.tv_nsec;
    return *this;
}
void Clock::Fill(struct tm& timeinfo, int& year, int& month, int& day, int& hour,
                 int& min, int& sec) {
    year = timeinfo.tm_year + 1900;
    month = timeinfo.tm_mon + 1;
    day = timeinfo.tm_mday;
    hour = timeinfo.tm_hour;
    min = timeinfo.tm_min;
    sec = timeinfo.tm_sec;
}

void Clock::FillTime(struct tm& timeinfo, int& hour, int& min, int& sec) {
    hour = timeinfo.tm_hour;
    min = timeinfo.tm_min;
    sec = timeinfo.tm_sec;
}

void Clock::Normalize_trading(int32_t real, int32_t& trading, int32_t& next_trading, int32_t& prev_trading, size_t trading_ind,
                              bool is_trading_day, bool is_prev_trading_day, time_t& sec,
                              struct tm& tradetimeinfo, std::vector<int32_t>& trading_list) {
    const time_t temp = 0;
    struct tm temp_tm, temp_tm2;
    temp_tm = *(localtime(&temp));
    temp_tm2 = *(localtime(&temp));
    strptime(std::to_string(real).c_str(), "%Y%m%d", &temp_tm);
    strptime(std::to_string(trading).c_str(), "%Y%m%d", &temp_tm2);
    auto sec2 = sec + mktime(&temp_tm2) - mktime(&temp_tm);
    tradetimeinfo = *(localtime(&sec2));

    // handle special situations in non-trading days
    if (!is_trading_day) {
        if (is_prev_trading_day && tradetimeinfo.tm_hour < 6)
            tradetimeinfo.tm_hour = tradetimeinfo.tm_hour;
        else
            tradetimeinfo.tm_hour = -1;
        return;
    }

    // come back to trading days
    if (tradetimeinfo.tm_hour < 6) {
        tradetimeinfo.tm_hour += 24;
    } else if (tradetimeinfo.tm_hour < 24 && tradetimeinfo.tm_hour >= 18) {
        if (is_trading_day) {
            prev_trading = trading_list[trading_ind];
            trading = trading_list[trading_ind + 1];
            next_trading = trading_list[trading_ind + 2];
        }
        strptime(std::to_string((long long)trading).c_str(), "%Y%m%d", &temp_tm2);
        sec2 = sec + mktime(&temp_tm2) - mktime(&temp_tm);
        tradetimeinfo = *(localtime(&sec2));
    }
}

/**
 * 20181129
 */
int32_t Clock::DateToIntReal() { return (timeinfo.tm_year + 1900) * 10000 + (timeinfo.tm_mon + 1) * 100 + timeinfo.tm_mday; }

int32_t Clock::PrevDateToIntReal() {
    time_t now = time(nullptr);
    now = now - (24 * 60 * 60);
    struct tm* t = localtime(&now);
    return (t->tm_year + 1900) * 10000 + (t->tm_mon + 1) * 100 + t->tm_mday;
}

int32_t Clock::DateToInt() { return year * 10000 + month * 100 + day; }

/**
 * hhmmss***
 */
int64_t Clock::TimeToInt() {
    return hour * 100 * 100 * 1000 + min * 100 * 1000 + sec * 1000 + int(nanoSecond / (1000 * 1000));
}

long Clock::TimeToIntNormalize() {
    long t = hour * 10000 * 1000 + min * 100 * 1000 + sec * 1000;
    auto millisec = nanoSecond / (1000.0 * 1000);
    if (millisec < 500)
        t += 0;
    else if (millisec >= 500)
        t += 500;
    return t;
}

long Clock::TradingTimeToInt() {
    if (DateToIntReal() == tradingDay)  // now is in trading day
        return TimeToInt();
    else {
        if (PrevDateToIntReal() == prevTradingDay && hour < 6)
            return TimeToInt();
        else
            return TimeToInt() * -1;
    }
}

std::string Clock::YearToStr() const {
    char timebuf[10];
    int ret = snprintf(timebuf, sizeof(timebuf), "%04d", year);
    if (ret < 0) {
        return "overflow";
    }
    return std::string(timebuf);
}

std::string Clock::MonthToStr() const {
    char timebuf[10];
    int ret = snprintf(timebuf, sizeof(timebuf), "%02d", month);
    if (ret < 0) {
        return "overflow";
    }
    return std::string(timebuf);
}

std::string Clock::DayToStr() const {
    char timebuf[10];
    int ret = snprintf(timebuf, sizeof(timebuf), "%02d", day);
    if (ret < 0) {
        return "overflow";
    }
    return std::string(timebuf);
}

std::string Clock::HourToStr() const {
    char timebuf[10];
    int ret = snprintf(timebuf, sizeof(timebuf), "%02d", hour);
    if (ret < 0) {
        return "overflow";
    }
    return std::string(timebuf);
}

std::string Clock::MinuteToStr() const {
    char timebuf[10];
    int ret = snprintf(timebuf, sizeof(timebuf), "%02d", min);
    if (ret < 0) {
        return "overflow";
    }
    return std::string(timebuf);
}

std::string Clock::SecondToStr() const {
    char timebuf[10];
    int ret = snprintf(timebuf, sizeof(timebuf), "%02d", sec);
    if (ret < 0) {
        return "overflow";
    }
    return std::string(timebuf);
}

std::string Clock::MillisecondToStr() const {
    char timebuf[10];
    int ret = snprintf(timebuf, sizeof(timebuf), "%03d", (int)round(core.tv_nsec / 1000 / 1000));
    if (ret < 0) {
        return "overflow";
    }
    return std::string(timebuf);
}

bool Clock::operator<(const Clock& right) const {
    if (core.tv_sec == right.core.tv_sec)
        return (core.tv_nsec < right.core.tv_nsec);
    else
        return core.tv_sec < right.core.tv_sec;
}

bool Clock::operator==(const Clock& right) const {
    return (core.tv_sec == right.core.tv_sec) && (core.tv_nsec == right.core.tv_nsec);
}

bool Clock::operator!=(const Clock& right) const { return !(this->operator==(right)); }

bool Clock::operator>(const Clock& right) const { return !(this->operator==(right)) && !(this->operator<(right)); }

bool Clock::operator>=(const Clock& right) const { return !(this->operator<(right)); }

bool Clock::operator<=(const Clock& right) const { return !(this->operator>(right)); }

void Clock::SetTimeInfo() {
    auto temp_tm = localtime(&core.tv_sec);
    timeinfo = *temp_tm;
}

void Clock::SetFromCore() {
    SetTimeInfo();
    AdjustTradingDay();
    if (isStrictTradingDay) {
        Normalize_trading(DateToIntReal(), tradingDay, nextTradingDay, prevTradingDay, tradingDayIndex, isTradingDay,
                          isPrevTradingDay, core.tv_sec, tradingtimeinfo, tradingDays);
        Fill(tradingtimeinfo, year, month, day, hour, min, sec);
    } else {
        Fill(timeinfo, year, month, day, hour, min, sec);
    }
    nanoSecond = core.tv_nsec;
}

void Clock::AdjustTradingDay() {
    if (tradingDays.empty()) {
        if (isStrictTradingDay) {
            ZLOG_THROW("TradingDay Clock cannot have empty trading day list");
        } else {
            return;
        }
    }

    long todayInt = DateToIntReal();
    long yesterdayInt = PrevDateToIntReal();

    // check trading day_list boundary
    if (tradingDays.front() > todayInt || tradingDays.back() < todayInt) {
        if (isStrictTradingDay) {
            ZLOG_THROW("TradingList not includes today");
        } else {
            SetTradingDayInfo(_gen_trading_days(timeinfo.tm_year + 1900));
        }
    }

    // found trading_ind with tradingDays[trading_ind] >= todayInt
    while (tradingDays[tradingDayIndex] > todayInt) {
        --tradingDayIndex;
    }
    while (tradingDays[tradingDayIndex] < todayInt) {
        ++tradingDayIndex;
    }

    tradingDay = tradingDays[tradingDayIndex];
    auto next_ind = tradingDayIndex > tradingDays.size() - 2 ? tradingDays.size() - 1 : tradingDayIndex + 1;
    nextTradingDay = tradingDays[next_ind];
    auto prev_ind = tradingDayIndex > 0 ? tradingDayIndex - 1 : 0;
    prevTradingDay = tradingDays[prev_ind];
    isTradingDay = (tradingDays[tradingDayIndex] == todayInt);
    isPrevTradingDay = (tradingDays[tradingDayIndex - 1] == yesterdayInt);
}



/**
 * example:
 * 12:30
 * 1:39:30
 * 1:39:29.300
 * 1:39:19 am (AM, a.m. A.M.)
 * @param literal
 * @return
 */
Clock HumanReadableTime(const std::string& literal) {
    std::string hour = "0";
    std::string min = "00";
    std::string sec = "00";
    std::string ms = "000";
    bool is_pm = false;

    std::string time_string = literal;
    zerg::ToLowerCase(time_string);
    std::string time;
    auto found1 = time_string.find("am");
    auto found2 = time_string.find("a.m");
    auto found3 = time_string.find("pm");
    auto found4 = time_string.find("p.m");

    size_t found;

    if (found1 != std::string::npos)
        time = time_string.substr(0, found1);
    else if (found2 != std::string::npos)
        time = time_string.substr(0, found2);
    else if (found3 != std::string::npos) {
        time = time_string.substr(0, found3);
        is_pm = true;
    } else if (found4 != std::string::npos) {
        time = time_string.substr(0, found4);
        is_pm = true;
    } else
        time = time_string;

    found = time.find(':');
    if (found == std::string::npos) {
        ZLOG_THROW("HumanReadableTime cannot recognize your time string: %s", literal.c_str());
    }

    hour = time.substr(0, found);
    if (std::stoi(hour) > 12 && is_pm) {
        ZLOG_THROW("HumanReadableTime finds your time string: %s to be illegal", literal.c_str());
    }

    if (is_pm) hour = std::to_string(std::stoi(hour) + 12);
    found2 = time.find(':', found + 1);
    if (found2 != std::string::npos) {
        min = time.substr(found + 1, found2 - found - 1);
        found3 = time.find('.', found2 + 1);
        if (found3 != std::string::npos) {
            sec = time.substr(found2 + 1, found3 - found2 - 1);

            ms = time.substr(found3 + 1, time.size() - found3 - 1);
        } else
            sec = time.substr(found2 + 1, time.size() - found2 - 1);
    } else
        min = time.substr(found + 1, time.size() - found - 1);

    if (std::stoi(min) >= 60) {
        ZLOG_THROW("HumanReadableTime illegal time: minute larger than 60");
    }

    if (std::stoi(sec) >= 60) {
        ZLOG_THROW("HumanReadableTime illegal time: second larger than 60");
    }

    if (std::stoi(ms) >= 1000) {
        ZLOG_THROW("HumanReadableTime illegal time: millisecond larger than 1000");
    }

    auto lit = zerg::Trim(hour) + ":" + zerg::Trim(min) + ":" + zerg::Trim(sec);
    auto clock = Clock(lit.c_str(), "%H:%M:%S");
    clock.nanoSecond = std::stoi(ms) * 1000 * 1000;
    clock.core.tv_nsec = clock.nanoSecond;
    return clock;
}

}
