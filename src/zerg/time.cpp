#include <zerg/string.h>
#include <zerg/time/time.h>
#include <iomanip>

namespace zerg {
long GetMicrosecondsSinceEpoch() {
    struct timeval tp;
    gettimeofday(&tp, nullptr);
    long ms = tp.tv_sec * 1000L * 1000L + tp.tv_usec;
    return ms;
}

struct timeval GetTimevalFromMicrosecondsSinceEpoch(long ms) {
    struct timeval tp;
    tp.tv_sec = ms / 1000 / 1000;
    tp.tv_usec = ms - tp.tv_sec * 1000L * 1000L;
    return tp;
}

struct tm GetTmFromMicrosecondsSinceEpoch(long ms) {
    struct timeval tp = GetTimevalFromMicrosecondsSinceEpoch(ms);
    auto nowtm = localtime(&tp.tv_sec);
    return *nowtm;
}

std::vector<int32_t> GenAllDaysInYear(int32_t year) {
    bool is_leap = (year % 4 == 0 ? (year % 100 == 0 ? (year % 400 == 0) : (true)) : (false));

    // start from Jan 1 of this year
    std::vector<int32_t> day_list;
    day_list.reserve(400);
    int32_t weekday[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int32_t day_of_mon[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (is_leap) day_of_mon[1] = 29;
    for (int m = 1; m <= 12; ++m) {
        for (int d = 1; d <= day_of_mon[m - 1]; ++d) {
            auto y = year;
            y -= m < 3;
            int32_t dayofweek = (y + y / 4 - y / 100 + y / 400 + weekday[m - 1] + d) % 7;
            if (dayofweek == 0 || dayofweek == 6) continue;
            day_list.push_back(year * 10000 + m * 100 + d);
        }
    }
    return day_list;
}

uint64_t HumanReadableMicrosecond(const std::string& str) {
    constexpr uint64_t _second = 1000ull * 1000ull;
    constexpr uint64_t _minute = 60ull * _second;
    constexpr uint64_t _hour = 60ull * _minute;
    std::string str_ = str;
    zerg::ToLowerCase(str_);
    uint64_t interval = 0;
    if (str_.find("milliseconds") != std::string::npos) {
        interval = 1000ull * std::stoull(str_.substr(0, str_.find("milliseconds")));
    } else if (str_.find("millisecond") != std::string::npos) {
        interval = 1000ull * std::stoull(str_.substr(0, str_.find("millisecond")));
    } else if (str_.find("ms") != std::string::npos) {
        interval = 1000ull * std::stoull(str_.substr(0, str_.find("ms")));
    } else if (str_.find("minutes") != std::string::npos) {
        interval = _minute * std::stoull(str_.substr(0, str_.find("minutes")));
    } else if (str_.find("minute") != std::string::npos) {
        interval = _minute * std::stoull(str_.substr(0, str_.find("minute")));
    } else if (str_.find("min") != std::string::npos) {
        interval = _minute * std::stoull(str_.substr(0, str_.find("min")));
    } else if (str_.find("hours") != std::string::npos) {
        interval = _hour * std::stoull(str_.substr(0, str_.find("hours")));
    } else if (str_.find("hour") != std::string::npos) {
        interval = _hour * std::stoull(str_.substr(0, str_.find("hour")));
    } else if (str_.find("macroseconds") != std::string::npos) {
        interval = std::stoull(str_.substr(0, str_.find("macroseconds")));
    } else if (str_.find("macrosecond") != std::string::npos) {
        interval = std::stoull(str_.substr(0, str_.find("macrosecond")));
    } else if (str_.find("seconds") != std::string::npos) {
        interval = _second * std::stoull(str_.substr(0, str_.find("seconds")));
    } else if (str_.find("second") != std::string::npos) {
        interval = _second * std::stoull(str_.substr(0, str_.find("second")));
    } else if (str_.find("sec") != std::string::npos) {
        interval = _second * std::stoull(str_.substr(0, str_.find("sec")));
    } else if (str_.find("quarter") != std::string::npos) {
        interval = 15ull * _minute * std::stoull(str_.substr(0, str_.find("quarter")));
    } else if (str_.find("us") != std::string::npos) {
        interval = std::stoull(str_.substr(0, str_.find("us")));
    } else if (str_.find('q') != std::string::npos) {
        interval = 15ull * _minute * std::stoull(str_.substr(0, str_.find('q')));
    } else if (str_.find('m') != std::string::npos) {
        interval = _minute * std::stoull(str_.substr(0, str_.find('m')));
    } else if (str_.find('s') != std::string::npos) {
        interval = _second * std::stoull(str_.substr(0, str_.find('s')));
    } else if (str_.find('h') != std::string::npos) {
        interval = _hour * std::stoull(str_.substr(0, str_.find('h')));
    } else if (not str.empty()) {
        interval = std::stoull(str_) * 1000ull;
    }
    return interval;
}

uint64_t HumanReadableMicrosecond(const char* interval_string) {
    return HumanReadableMicrosecond(std::string(interval_string));
}

uint64_t HumanReadableMillisecond(const std::string& interval_string) {
    return HumanReadableMicrosecond(interval_string) / 1000;
}

uint64_t HumanReadableMillisecond(const char* interval_string) {
    return HumanReadableMillisecond(std::string(interval_string));
}

double timespec2double(const timespec& ts) {
    long usec = ts.tv_nsec / 1000;
    return static_cast<double>(ts.tv_sec) + (static_cast<double>(usec) / 1000000.0);
}

void double2timespec(double t, timespec& ts) {
    ts.tv_sec = static_cast<time_t>(t);
    double dt = (t - static_cast<double>(ts.tv_sec)) * 1000000.0;
    dt = floor(dt + 0.5);
    long usec = static_cast<long>(dt);
    ts.tv_nsec = usec * 1000;
}

double timeval2double(const timeval& ts) {
    return static_cast<double>(ts.tv_sec) + (static_cast<double>(ts.tv_usec) / 1000000.0);
}

uint64_t timespec2nanos(const timespec& ts) {
    return static_cast<uint64_t>(ts.tv_sec) * oneSecondNano + static_cast<uint64_t>(ts.tv_nsec);
}

void nanos2timespec(uint64_t t, timespec& ts) {
    ts.tv_sec = static_cast<long>(t / oneSecondNano);
    ts.tv_nsec = static_cast<long>(t % oneSecondNano);
}

int timespec_cmp(const timespec& t1, const timespec& t2) {
    if (t1.tv_sec < t2.tv_sec)
        return -1;
    else if (t1.tv_sec > t2.tv_sec)
        return 1;
    else if (t1.tv_nsec < t2.tv_nsec)
        return -1;
    else if (t1.tv_nsec > t2.tv_nsec)
        return 1;
    else
        return 0;
}

int64_t nanoSinceEpoch() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<int64_t>(timespec2nanos(ts));
}

uint64_t nanoSinceEpochU() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return timespec2nanos(ts);
}

uint64_t ntime() { return nanoSinceEpochU(); }
long utime() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

// YYYYMMDD format
uint32_t nano2date(uint64_t nano) {
    if (nano == 0) {
        nano = nanoSinceEpochU();
    }
    time_t epoch = static_cast<long>(nano / oneSecondNano);
    struct tm tm{};
    gmtime_r(&epoch, &tm);
    return static_cast<uint32_t>((tm.tm_year + 1900) * 10000 + (tm.tm_mon + 1) * 100 + tm.tm_mday);
}

std::string time_t2string(const time_t ct) {
    if (!ct) return "N/A";
    struct tm tm{};
    localtime_r(&ct, &tm);
    char buffer[21];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    std::snprintf(buffer, sizeof buffer, "%4u-%02u-%02u %02u:%02u:%02u", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
#pragma GCC diagnostic pop
    return string(buffer);
}

int64_t time_t2hms(const time_t ct) {
    struct tm tm{};
    localtime_r(&ct, &tm);
    return (tm.tm_hour * 10000 + tm.tm_min * 100 + tm.tm_sec);
}

void split_hms(int hms, int& h, int& m, int& s) {
    h = hms / 10000;
    m = (hms % 10000) / 100;
    s = hms % 100;
}

std::string ntime2string(uint64_t nano) {
    time_t epoch = static_cast<long>(nano / oneSecondNano);
    return time_t2string(epoch);
}
time_t time_t_from_ymdhms(int ymd, int hms) {
    struct tm tm{};
    tm.tm_mday = ymd % 100;
    ymd /= 100;
    tm.tm_mon = ymd % 100 - 1;
    tm.tm_year = ymd / 100 - 1900;
    tm.tm_sec = hms % 100;
    hms /= 100;
    tm.tm_min = hms % 100;
    tm.tm_hour = hms / 100;
    return mktime(&tm);
}
uint64_t ntime_from_double(double ymdhms) {
    int date = static_cast<int>(ymdhms);
    int hms = static_cast<int>((ymdhms - date) * 1000000);
    return oneSecondNano * static_cast<uint64_t>(time_t_from_ymdhms(date, hms));
}

std::string timeval2string(const struct timeval& tv) {
    struct tm tm{};
    localtime_r(&tv.tv_sec, &tm);
    char buffer[24];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    std::snprintf(buffer, sizeof buffer, "%4u-%02u-%02u %02u:%02u:%02u.%03u", tm.tm_year + 1900, tm.tm_mon + 1,
                  tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<uint16_t>(tv.tv_usec / 1000));
#pragma GCC diagnostic pop
    return string(buffer);
}

std::string timespec2string(const timespec& ts) {
    struct tm tm{};
    localtime_r(&ts.tv_sec, &tm);
    char buffer[24];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    std::snprintf(buffer, sizeof buffer, "%4u-%02u-%02u %02u:%02u:%02u.%03u", tm.tm_year + 1900, tm.tm_mon + 1,
                  tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<uint16_t>(ts.tv_nsec / 1000000));
#pragma GCC diagnostic pop
    return string(buffer);
}

bool string_2_second_intraday(const string& str, const string& format, int& secs) {
    if (format == "HHMMSS" && str.length() >= 4) {
        const char* p = str.c_str();
        int hour = (10 * (p[0] - '0')) + (p[1] - '0');
        int minute = (10 * (p[2] - '0')) + (p[3] - '0');
        int second = 0;
        if (str.length() >= 6) second = (10 * (p[0] - '0')) + (p[1] - '0');

        if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59) {
            secs = (hour * 60 * 60) + minute * 60 + second;
            return true;
        }
    }
    return false;
}

bool string_2_microsecond_intraday(const string& str, const string& format, int& msecs) {
    if (format == "HHMMSS.MMM") {
        int secs = 0;
        if (string_2_second_intraday(str, "HHMMSS", secs)) {
            msecs = secs * 1000;
            if (str.length() == 10) {
                const char* p = str.c_str();
                msecs += (100 * (p[7] - '0')) + (10 * (p[8] - '0')) + (p[9] - '0');
            }
            return true;
        }
    }
    return false;
}

timeval now_timeval() {
    timeval tv;
    gettimeofday(&tv, nullptr);
    return tv;
}

std::string now_local_string() {
    timeval tv;
    gettimeofday(&tv, nullptr);
    return timeval2string(tv);
}

int64_t now_cob() {
    time_t tNow = time(nullptr);
    struct tm tm{};
    localtime_r(&tNow, &tm);
    return (tm.tm_year + 1900) * 10000 + (tm.tm_mon + 1) * 100 + tm.tm_mday;
}

int64_t now_HMS() {
    time_t tNow = (time_t)time(nullptr);
    struct tm tm{};
    localtime_r(&tNow, &tm);
    return tm.tm_hour * 10000 + tm.tm_min * 100 + tm.tm_sec;
}

std::string now_string() {
    time_t tNow = time(nullptr);
    struct tm tm{};
    localtime_r(&tNow, &tm);
    char buffer[16];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    std::snprintf(buffer, sizeof buffer, "%4u%02u%02u.%02u%02u%02u", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
#pragma GCC diagnostic pop
    return string(buffer);
}
/**
 * "23:59:59" to 235959
 */
int intraday_time_HMS(const char* str) {
    return (str[0] - '0') * 100000 + (str[1] - '0') * 10000 + (str[3] - '0') * 1000 + (str[4] - '0') * 100 +
           (str[6] - '0') * 10 + (str[7] - '0');
}
int intraday_time_HMS(const string& str) { return intraday_time_HMS(str.c_str()); }

int64_t usec_HMS(const char* str, int64_t ms) {
    int64_t h = (str[0] - '0') * 10 + (str[1] - '0');
    int64_t m = (str[3] - '0') * 10 + (str[4] - '0');
    int64_t s = (str[6] - '0') * 10 + (str[7] - '0');
    return (h * 3600L + m * 60L + s) * 1000000L + ms * 1000L;
}
int64_t usec_HMS(const string& str, int64_t ms) { return usec_HMS(str.c_str(), ms); }

/**
 * "23:59" to 2359
 */
int intraday_time_HM(const char* str) {
    return (str[0] - '0') * 1000 + (str[1] - '0') * 100 + (str[3] - '0') * 10 + (str[4] - '0');
}
int intraday_time_HM(const string& str) { return intraday_time_HM(str.c_str()); }
/**
 * "20171112" to 20171112
 */
int cob(const char* str) {
    return (str[0] - '0') * 10000000 + (str[1] - '0') * 1000000 + (str[2] - '0') * 100000 + (str[3] - '0') * 10000 +
           (str[4] - '0') * 1000 + (str[5] - '0') * 100 + (str[6] - '0') * 10 + (str[7] - '0');
}
int cob(const string& str) { return cob(str.c_str()); }

/**
 * "2017-11-12" to 20171112
 */
int cob_from_dash(const char* str) {
    return (str[0] - '0') * 10000000 + (str[1] - '0') * 1000000 + (str[2] - '0') * 100000 + (str[3] - '0') * 10000 +
           (str[5] - '0') * 1000 + (str[6] - '0') * 100 + (str[8] - '0') * 10 + (str[9] - '0');
}
int cob_from_dash(const string& str) { return cob_from_dash(str.c_str()); }

/**
 * "20171112 23:59:59" to time_t
 */
double time_string2double(const string& str) {
    std::tm t = {};
    std::istringstream ss(str);
    ss >> std::get_time(&t, "%Y%m%d %H:%M:%S");
    return mktime(&t);
}

std::string replace_time_placeholder(const std::string& str, int d, int32_t t) {
    std::string year, month, day, timestamp, timestamp_sss;
    char buffer[100], time_buffer[100];
    if (d <= 0) {
        time_t t_ = time(nullptr);
        struct tm* time_info = localtime(&t_);
        strftime(buffer, 80, "%Y", time_info);
        year = buffer;
        strftime(buffer, 80, "%m", time_info);
        month = buffer;
        strftime(buffer, 80, "%d", time_info);
        day = buffer;
    } else {
        sprintf(buffer, "%04d", d / 10000);
        year = buffer;
        sprintf(buffer, "%02d", (d % 10000) / 100);
        month = buffer;
        sprintf(buffer, "%02d", d % 100);
        day = buffer;
    }
    if (t >= 0) {
        sprintf(time_buffer, "%06d", t / 1000);
        timestamp = time_buffer;
        sprintf(time_buffer, "%09d", t);
        timestamp_sss = time_buffer;
    } else {
        time_t t_ = time(nullptr);
        struct tm* timeinfo = localtime(&t_);
        strftime(time_buffer, 80, "%H%M%S", timeinfo);
        timestamp = time_buffer;
        timeval tt;
        gettimeofday(&tt, nullptr);
        sprintf(time_buffer, "%03d", (int)tt.tv_usec / 1000);
        timestamp_sss = timestamp + time_buffer;
    }
    std::string ret = str;
    ReplaceAll(ret, "${YYYY}", year);
    ReplaceAll(ret, "${MM}", month);
    ReplaceAll(ret, "${DD}", day);
    ReplaceAll(ret, "${YYYYMMDD}", year + month + day);
    ReplaceAll(ret, "${YYYYMM}", year + month);
    ReplaceAll(ret, "${MMDD}", month + day);
    ReplaceAll(ret, "${HHMMSS}", timestamp);
    ReplaceAll(ret, "${HHMMSSmmm}", timestamp_sss);
    return ret;
}

/**
 * rdtscp has less jitter but more error
 * both rdtsc and rdtscp has error less than 15ns
 * When using rdtsc, you need to use cpuid to ensure that no additional instructions are in the execution pipeline.
 * The rdtscp instruction flushes the pipeline intrinsically.
 */
uint64_t rdtsc() {
    unsigned long rax, rdx;
    asm volatile("rdtsc\n" : "=a"(rax), "=d"(rdx));
    return (rdx << 32) + rax;
}
uint64_t rdtscp() {
    unsigned int aux;
    unsigned long rax, rdx;
    asm volatile("rdtscp\n" : "=a"(rax), "=d"(rdx), "=c"(aux));
    return (rdx << 32) + rax;
}
int GetNextDate(int cob) {
    struct tm tm{};
    tm.tm_mday = cob % 100;
    cob /= 100;
    tm.tm_mon = cob % 100 - 1;
    tm.tm_year = cob / 100 - 1900;
    tm.tm_sec = 0;
    tm.tm_min = 0;
    tm.tm_hour = 12;
    time_t t = mktime(&tm) + 24 * 60 * 60;
    struct tm tm1{};
    localtime_r(&t, &tm1);
    return (tm1.tm_year + 1900) * 10000 + (tm1.tm_mon + 1) * 100 + tm1.tm_mday;
}
}
