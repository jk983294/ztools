#pragma once

#include <sys/time.h>
#include <cmath>
#include <cstring>
#include <ctime>
#include <fstream>
#include <vector>
#include <zerg/log.h>
#include <zerg/string.h>
#include <zerg/time/time.h>

using std::string;

namespace zerg {

namespace TimePolicy {
class Normal;
class TradingDay;
}  // namespace TimePolicy

template <typename policy = TimePolicy::Normal>
class Clock {
public:
    bool IsTradingDay() {
        if (tradingDays.empty()) ZLOG_THROW("TradingDayList is empty, cannot decide if trading day.");
        return isTradingDay;
    }

protected:
    std::vector<long> tradingDays;
    size_t tradingDayIndex;
    bool isTradingDay;
    bool isPrevTradingDay;

public:
    long tradingDay;
    long nextTradingDay;
    long prevTradingDay;

public:
    timespec core;
    int year{0};
    int month{0};
    int day{0};
    int hour{0};
    int min{0};
    int sec{0};
    long nanoSecond{0};
    long elapse{0};  // store time elapsed during start and

    Clock() {
        tradingDayIndex = 0;
        clock_gettime(CLOCK_REALTIME, &core);
        SetTimeInfo();

        if (policy::IsTrading()) {
            auto tl1 = GenAllDaysInYear(timeinfo.tm_year + 1900 - 1);
            auto tl2 = GenAllDaysInYear(timeinfo.tm_year + 1900);
            auto tl3 = GenAllDaysInYear(timeinfo.tm_year + 1900 + 1);

            std::move(tl2.begin(), tl2.end(), std::back_inserter(tl1));
            std::move(tl3.begin(), tl3.end(), std::back_inserter(tl1));

            SetTradingDayInfo(tl1);
            SetFromCore();
        }
    }

    Clock(const std::string& filename) {
        tradingDayIndex = 0;
        clock_gettime(CLOCK_REALTIME, &core);
        SetTimeInfo();

        SetTradingDayInfo(filename);
        SetFromCore();
    }

    Clock(const std::vector<long>& tl) {
        tradingDayIndex = 0;
        clock_gettime(CLOCK_REALTIME, &core);
        SetTimeInfo();

        SetTradingDayInfo(tl);
        SetFromCore();
    }

    Clock(const Clock& clock) {
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
        if (policy::IsTrading()) {
            if (tradingDays.empty()) ZLOG_THROW("Tradingday list cannot be empty for trading clock");
        }
        if (!tradingDays.empty()) SetTradingDayInfo(tradingDays);

        SetFromCore();
    }

    Clock& operator=(const Clock& clock) {
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
        if (policy::IsTrading()) {
            if (tradingDays.empty()) ZLOG_THROW("Trading day list cannot be empty for trading clock");
        }
        if (!tradingDays.empty()) SetTradingDayInfo(tradingDays);
        SetFromCore();
        return *this;
    }

    // use clock to represent a date or time
    Clock(const char* string_time, const char* format) {
        if (policy::IsTrading()) {
            ZLOG_THROW("Trading Clock must be supplied with trading day list");
        }

        // first make timeinfo back to epoch
        const time_t temp = 0;
        auto temp_tm = localtime(&temp);
        timeinfo = *temp_tm;
        year = month = day = hour = min = sec = nanoSecond = 0;

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
    Clock(const char* string_time, const char* format, std::vector<long>& list) {
        // first make timeinfo back to epoch
        const time_t temp = 0;
        auto temp_tm = localtime(&temp);
        timeinfo = *temp_tm;
        year = month = day = hour = min = sec = nanoSecond = 0;

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
    Clock(const char* string_time, const char* format, const char* filename) {
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

    virtual ~Clock() = default;

    //! set trading day list
    void SetTradingDayInfo(const std::vector<long>& tl) {
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
    void SetTradingDayInfo(const std::string& filename) {
        std::vector<long> tl;
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
    Clock& Set(time_t epoch_time = 0) {
        memset(&core, 0, sizeof(timespec));
        core.tv_sec = epoch_time;
        auto temp_tm = localtime(&core.tv_sec);
        timeinfo = *temp_tm;
        SetFromCore();
        return *this;
    }

    // equals Set(0)
    Clock& Reset() {
        Set(0);
        return *this;
    }

    Clock& Update(const char* string_time, const char* format) {
        // first make timeinfo back to epoch
        time_t temp = 0;
        auto temp_tm = localtime(&temp);
        timeinfo = *temp_tm;
        year = month = day = hour = min = sec = nanoSecond = 0;

        // parse it now
        if (strptime(string_time, format, &timeinfo) == nullptr) {
            ZLOG_THROW("Clock cannot properly parse time string: %s with format: %s", string_time, format);
        }

        core.tv_sec = mktime(&timeinfo);
        core.tv_nsec = 0;
        SetFromCore();
    }

    // update time and date to current timestamp
    Clock& Update() {
        clock_gettime(CLOCK_REALTIME, &core);
        SetFromCore();
        return *this;
    }

    // just update time
    Clock& UpdateTime() {
        clock_gettime(CLOCK_REALTIME, &core);
        SetTimeInfo();
        policy::Normalize(DateToIntReal(), tradingDay, nextTradingDay, prevTradingDay, tradingDayIndex, isTradingDay,
                          isPrevTradingDay, core.tv_sec, timeinfo, tradingtimeinfo, tradingDays);

        policy::FillTime(timeinfo, tradingtimeinfo, hour, min, sec);
        nanoSecond = core.tv_nsec;
        return *this;
    }

    /**
     * 20181129
     */
    long DateToIntReal() { return (timeinfo.tm_year + 1900) * 10000 + (timeinfo.tm_mon + 1) * 100 + timeinfo.tm_mday; }

    long PrevDateToIntReal() {
        time_t now = time(nullptr);
        now = now - (24 * 60 * 60);
        struct tm* t = localtime(&now);
        return (t->tm_year + 1900) * 10000 + (t->tm_mon + 1) * 100 + t->tm_mday;
    }

    long DateToInt() { return year * 10000 + month * 100 + day; }

    /**
     * hhmmss***
     */
    long TimeToInt() {
        return hour * 100 * 100 * 1000 + min * 100 * 1000 + sec * 1000 + int(nanoSecond / (1000 * 1000));
    }

    long TimeToIntNormalize() {
        long t = hour * 10000 * 1000 + min * 100 * 1000 + sec * 1000;
        auto millisec = nanoSecond / (1000.0 * 1000);
        if (millisec < 500)
            t += 0;
        else if (millisec >= 500)
            t += 500;
        return t;
    }

    long TradingTimeToInt() {
        if (DateToIntReal() == tradingDay)  // now is in trading day
            return TimeToInt();
        else {
            if (PrevDateToIntReal() == prevTradingDay && hour < 6)
                return TimeToInt();
            else
                return TimeToInt() * -1;
        }
    }

    std::string YearToStr() const {
        char timebuf[10];
        int ret = snprintf(timebuf, sizeof(timebuf), "%04d", year);
        if (ret < 0) {
            return "overflow";
        }
        return std::string(timebuf);
    }

    std::string MonthToStr() const {
        char timebuf[10];
        int ret = snprintf(timebuf, sizeof(timebuf), "%02d", month);
        if (ret < 0) {
            return "overflow";
        }
        return std::string(timebuf);
    }

    std::string DayToStr() const {
        char timebuf[10];
        int ret = snprintf(timebuf, sizeof(timebuf), "%02d", day);
        if (ret < 0) {
            return "overflow";
        }
        return std::string(timebuf);
    }

    std::string HourToStr() const {
        char timebuf[10];
        int ret = snprintf(timebuf, sizeof(timebuf), "%02d", hour);
        if (ret < 0) {
            return "overflow";
        }
        return std::string(timebuf);
    }

    std::string MinuteToStr() const {
        char timebuf[10];
        int ret = snprintf(timebuf, sizeof(timebuf), "%02d", min);
        if (ret < 0) {
            return "overflow";
        }
        return std::string(timebuf);
    }

    std::string SecondToStr() const {
        char timebuf[10];
        int ret = snprintf(timebuf, sizeof(timebuf), "%02d", sec);
        if (ret < 0) {
            return "overflow";
        }
        return std::string(timebuf);
    }

    std::string MillisecondToStr() const {
        char timebuf[10];
        int ret = snprintf(timebuf, sizeof(timebuf), "%03d", (int)round(core.tv_nsec / 1000 / 1000));
        if (ret < 0) {
            return "overflow";
        }
        return std::string(timebuf);
    }

    class Clock operator+(const int64_t interval) {
        Clock ret = *this;
        ret.core.tv_sec += (int64_t)floor(interval / 1000);
        ret.core.tv_nsec += (interval - 1000 * (int64_t)floor(interval / 1000)) * 1000 * 1000;
        if (ret.core.tv_nsec >= 1000 * 1000 * 1000) {
            ret.core.tv_sec += 1;
            ret.core.tv_nsec -= 1000 * 1000 * 1000;
        }
        ret.nanoSecond = ret.core.tv_nsec;
        auto temp_tm = localtime(&ret.core.tv_sec);
        ret.timeinfo = *temp_tm;

        ret.year = ret.timeinfo.tm_year + 1900;
        ret.month = ret.timeinfo.tm_mon + 1;
        ret.day = ret.timeinfo.tm_mday;
        ret.hour = ret.timeinfo.tm_hour;
        ret.min = ret.timeinfo.tm_min;
        ret.sec = ret.timeinfo.tm_sec;
        return ret;
    }

    class Clock operator+=(const int64_t interval) {
        core.tv_sec += (int64_t)floor(interval / 1000);
        nanoSecond += (interval - 1000 * (int64_t)floor(interval / 1000)) * 1000 * 1000;
        if (nanoSecond >= 1000 * 1000 * 1000) {
            core.tv_sec += 1;
            nanoSecond -= 1000 * 1000 * 1000;
        }
        core.tv_nsec = nanoSecond;
        auto temp_tm = localtime(&core.tv_sec);
        timeinfo = *temp_tm;
        year = timeinfo.tm_year + 1900;
        month = timeinfo.tm_mon + 1;
        day = timeinfo.tm_mday;
        hour = timeinfo.tm_hour;
        min = timeinfo.tm_min;
        sec = timeinfo.tm_sec;
        return *this;
    }

    class Clock operator+(const double interval_f) {
        long interval = lround(interval_f / 1.1);
        if (tradingDayIndex + interval > tradingDays.size() - 1) {
            ZLOG_THROW("clock trading day increment exceeds boundary");
        }
        long target = tradingDays[tradingDayIndex + interval];
        long today = tradingDays[tradingDayIndex];
        auto target_day = Clock(std::to_string((long long)target).c_str(), "%Y%m%d");
        auto today_day = Clock(std::to_string((long long)today).c_str(), "%Y%m%d");
        long interval_sec = target_day.core.tv_sec - today_day.core.tv_sec;

        Clock ret = *this;
        ret.core.tv_sec += interval_sec;
        auto temp_tm = localtime(&ret.core.tv_sec);
        ret.timeinfo = *temp_tm;
        ret.year = ret.timeinfo.tm_year + 1900;
        ret.month = ret.timeinfo.tm_mon + 1;
        ret.day = ret.timeinfo.tm_mday;
        ret.hour = ret.timeinfo.tm_hour;
        ret.min = ret.timeinfo.tm_min;
        ret.sec = ret.timeinfo.tm_sec;
        return ret;
    }

    class Clock operator-(const int64_t interval) {
        Clock ret = *this;
        ret.core.tv_sec -= (int64_t)floor(interval / 1000);
        int64_t nano = (interval - 1000 * (int64_t)floor(interval / 1000)) * 1000 * 1000;
        if (ret.core.tv_nsec < nano) {
            ret.core.tv_sec -= 1;  // borrow 1 sec
            ret.core.tv_nsec = ret.core.tv_nsec + 1000 * 1000 * 1000 - nano;
        } else
            ret.core.tv_nsec -= nano;
        ret.nanoSecond = ret.core.tv_nsec;

        auto temp_tm = localtime(&ret.core.tv_sec);
        ret.timeinfo = *temp_tm;
        ret.year = ret.timeinfo.tm_year + 1900;
        ret.month = ret.timeinfo.tm_mon + 1;
        ret.day = ret.timeinfo.tm_mday;
        ret.hour = ret.timeinfo.tm_hour;
        ret.min = ret.timeinfo.tm_min;
        ret.sec = ret.timeinfo.tm_sec;
        return ret;
    }

    class Clock operator-=(const int64_t interval) {
        core.tv_sec -= (int64_t)floor(interval / 1000);
        int64_t nano = (interval - 1000 * (int64_t)floor(interval / 1000)) * 1000 * 1000;
        if (core.tv_nsec < nano) {
            core.tv_sec -= 1;  // borrow 1 sec
            core.tv_nsec = core.tv_nsec + 1000 * 1000 * 1000 - nano;
        } else {
            core.tv_nsec -= nano;
        }
        nanoSecond = core.tv_nsec;

        auto temp_tm = localtime(&core.tv_sec);
        timeinfo = *temp_tm;
        year = timeinfo.tm_year + 1900;
        month = timeinfo.tm_mon + 1;
        day = timeinfo.tm_mday;
        hour = timeinfo.tm_hour;
        min = timeinfo.tm_min;
        sec = timeinfo.tm_sec;
        return *this;
    }

    bool operator<(const Clock& right) {
        if (core.tv_sec == right.core.tv_sec)
            return (core.tv_nsec < right.core.tv_nsec);
        else
            return core.tv_sec < right.core.tv_sec;
    }

    bool operator==(const Clock& right) {
        return (core.tv_sec == right.core.tv_sec) && (core.tv_nsec == right.core.tv_nsec);
    }

    bool operator!=(const Clock& right) { return !(this->operator==(right)); }

    bool operator>(const Clock& right) { return !(this->operator==(right)) && !(this->operator<(right)); }

    bool operator>=(const Clock& right) { return !(this->operator<(right)); }

    bool operator<=(const Clock& right) { return !(this->operator>(right)); }

private:
    void SetTimeInfo() {
        auto temp_tm = localtime(&core.tv_sec);
        timeinfo = *temp_tm;
    }

    void SetFromCore() {
        SetTimeInfo();
        AdjustTradingDay();
        policy::Normalize(DateToIntReal(), tradingDay, nextTradingDay, prevTradingDay, tradingDayIndex, isTradingDay,
                          isPrevTradingDay, core.tv_sec, timeinfo, tradingtimeinfo, tradingDays);

        policy::Fill(timeinfo, tradingtimeinfo, year, month, day, hour, min, sec);
        nanoSecond = core.tv_nsec;
    }

    long adjust_last_hour;
    long adjust_last_date;

    void AdjustTradingDay() {
        if (tradingDays.empty()) {
            if (policy::IsTrading()) {
                ZLOG_THROW("TradingDay Clock cannot have empty trading day list");
            } else {
                return;
            }
        }

        long todayInt = DateToIntReal();
        long yesterdayInt = PrevDateToIntReal();

        // check trading day_list boundary
        if (tradingDays[0] > todayInt) {
            ZLOG_THROW("TradingList not includes today");
        }

        else if (tradingDays[tradingDays.size() - 1] < todayInt) {
            ZLOG_THROW("TradingList not includes today");
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

protected:
    struct tm timeinfo;
    struct tm tradingtimeinfo;
};

namespace TimePolicy {
class Normal {
public:
    static bool IsTrading() { return false; }

    static void Normalize(long real, long& trading, long& next_trading, long& prev_trading, const size_t trading_ind,
                          bool& is_trading_day, bool& is_prev_trading_day, time_t& sec, struct tm& timeinfo,
                          struct tm& tradetimeinfo, std::vector<long>& trading_list) {}

    static void Fill(struct tm& timeinfo, struct tm& tradetimeinfo, int& year, int& month, int& day, int& hour,
                     int& min, int& sec) {
        year = timeinfo.tm_year + 1900;
        month = timeinfo.tm_mon + 1;
        day = timeinfo.tm_mday;
        hour = timeinfo.tm_hour;
        min = timeinfo.tm_min;
        sec = timeinfo.tm_sec;
    }
    static void FillTime(struct tm& timeinfo, struct tm& tradetimeinfo, int& hour, int& min, int& sec) {
        hour = timeinfo.tm_hour;
        min = timeinfo.tm_min;
        sec = timeinfo.tm_sec;
    }
};

class TradingDay {
public:
    static bool IsTrading() { return true; }

    static void Normalize(long real, long& trading, long& next_trading, long& prev_trading, const size_t trading_ind,
                          bool& is_trading_day, bool is_prev_trading_day, time_t& sec, struct tm& timeinfo,
                          struct tm& tradetimeinfo, std::vector<long>& trading_list) {
        const time_t temp = 0;
        struct tm temp_tm, temp_tm2;  // temp_tm3;
        temp_tm = *(localtime(&temp));
        temp_tm2 = *(localtime(&temp));
        strptime(std::to_string((long long)real).c_str(), "%Y%m%d", &temp_tm);
        strptime(std::to_string((long long)trading).c_str(), "%Y%m%d", &temp_tm2);
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

    static void Fill(struct tm& timeinfo, struct tm& tradetimeinfo, int& year, int& month, int& day, int& hour,
                     int& min, int& sec) {
        year = tradetimeinfo.tm_year + 1900;
        month = tradetimeinfo.tm_mon + 1;
        day = tradetimeinfo.tm_mday;
        hour = tradetimeinfo.tm_hour;

        min = tradetimeinfo.tm_min;
        sec = tradetimeinfo.tm_sec;
    }

    static void FillTime(struct tm& timeinfo, struct tm& tradetimeinfo, int& hour, int& min, int& sec) {
        hour = tradetimeinfo.tm_hour;
        min = tradetimeinfo.tm_min;
        sec = tradetimeinfo.tm_sec;
    }
};
}  // namespace TimePolicy

/**
 * example:
 * 12:30
 * 1:39:30
 * 1:39:29.300
 * 1:39:19 am (AM, a.m. A.M.)
 * @param literal
 * @return
 */
inline Clock<> HumanReadableTime(const std::string& literal);
}
