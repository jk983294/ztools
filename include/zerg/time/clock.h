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

struct Clock {
    std::vector<int32_t> tradingDays;
    size_t tradingDayIndex{0};
    bool isStrictTradingDay{false};
    bool isTradingDay{false};
    bool isPrevTradingDay{false};
    int32_t tradingDay{0};
    int32_t nextTradingDay{0};
    int32_t prevTradingDay{0};
    timespec core;
    int32_t year{0};
    int32_t month{0};
    int32_t day{0};
    int32_t hour{0};
    int32_t min{0};
    int32_t sec{0};
    int32_t adjust_last_hour{0};
    int64_t nanoSecond{0};
    int64_t elapse{0};  // store time elapsed during start and

    Clock(bool _isStrictTradingDay = false);
    bool IsTradingDay() const;
    Clock(const std::string& filename);
    Clock(const std::vector<int32_t>& tl);

    Clock(const Clock& clock);

    Clock& operator=(const Clock& clock);

    // use clock to represent a date or time
    Clock(const char* string_time, const char* format);

    // use clock to represent a date or time
    Clock(const char* string_time, const char* format, std::vector<int32_t>& list);

    // use clock to represent a date or time
    Clock(const char* string_time, const char* format, const char* filename);

    //! set trading day list
    void SetTradingDayInfo(const std::vector<int32_t>& tl);

    // set trading day list
    void SetTradingDayInfo(const std::string& filename);

    // set clock from epoch time long
    Clock& Set(time_t epoch_time = 0);

    Clock& Reset();

    Clock& Update(const char* string_time, const char* format);

    // update time and date to current timestamp
    Clock& Update();

    // just update time
    Clock& UpdateTime();

    static void Normalize_trading(int32_t real, int32_t& trading, int32_t& next_trading, int32_t& prev_trading,
                                  size_t trading_ind, bool is_trading_day, bool is_prev_trading_day, time_t& sec,
                                  struct tm& tradetimeinfo, std::vector<int32_t>& trading_list);
    static void Fill(struct tm& timeinfo, int& year, int& month, int& day, int& hour,
                     int& min, int& sec);

    static void FillTime(struct tm& timeinfo, int& hour, int& min, int& sec);

    int32_t DateToIntReal(); // YYYYMMDD

    int32_t PrevDateToIntReal();

    int32_t DateToInt();

    int64_t TimeToInt(); // hhmmss***

    long TimeToIntNormalize();

    long TradingTimeToInt();

    std::string YearToStr() const;

    std::string MonthToStr() const;

    std::string DayToStr() const;

    std::string HourToStr() const;

    std::string MinuteToStr() const;

    std::string SecondToStr() const;

    std::string MillisecondToStr() const;

    bool operator<(const Clock& right) const;

    bool operator==(const Clock& right) const;

    bool operator!=(const Clock& right) const;

    bool operator>(const Clock& right) const;

    bool operator>=(const Clock& right) const;

    bool operator<=(const Clock& right) const;

private:
    void SetTimeInfo();
    void SetFromCore();
    void AdjustTradingDay();

    struct tm timeinfo;
    struct tm tradingtimeinfo;
};

std::string ReplaceSpecialTimeHolderCopy(const std::string& str, const Clock& clock);
std::string ReplaceSpecialTimeHolder(std::string& str, const Clock& clock);
/**
 * example:
 * 12:30
 * 1:39:30
 * 1:39:29.300
 * 1:39:19 am (AM, a.m. A.M.)
 * @param literal
 * @return
 */
Clock HumanReadableTime(const std::string& literal);
}
