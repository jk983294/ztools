#pragma once

#include <sys/time.h>
#include <cmath>
#include <cstring>
#include <ctime>
#include <string>

using std::string;

namespace zerg {
constexpr int day_night_split = 180000000;  // ms 18:00:00 000
constexpr int day_start_split = 60000000;   // ms 6:00:00 000
constexpr int one_day_long = 240000000;
constexpr uint64_t oneSecondNano{1000000000};

double timespec2double(const timespec& ts);

double timeval2double(const timeval& ts);

void double2timespec(double t, timespec& ts);

uint64_t timespec2nanos(const timespec& ts);

void nanos2timespec(uint64_t t, timespec& ts);

int timespec_cmp(const timespec& t1, const timespec& t2);

int64_t nanoSinceEpoch();
uint64_t nanoSinceEpochU();

long GetMicrosecondsSinceEpoch();
struct timeval GetTimevalFromMicrosecondsSinceEpoch(long ms);
struct tm GetTmFromMicrosecondsSinceEpoch(long ms) ;

uint64_t ntime();
long utime();

// YYYYMMDD format
uint32_t nano2date(uint64_t nano = 0);

std::string time_t2string(const time_t ct);

std::string ntime2string(uint64_t nano);
time_t time_t_from_ymdhms(int ymd, int hms);
uint64_t ntime_from_double(double ymdhms);

std::string timeval2string(const struct timeval& tv);

std::string timespec2string(const timespec& ts);

bool string_2_second_intraday(const string& str, const string& format, int& secs);

bool string_2_microsecond_intraday(const string& str, const string& format, int& msecs);

timeval now_timeval();

std::string now_string();
std::string now_local_string();

int64_t now_cob();
int64_t now_HMS();

inline int year_from_cob(int cob) { return cob / 10000; }
inline int month_from_cob(int cob) { return (cob / 100) % 100; }
inline int day_from_cob(int cob) { return cob % 100; }

inline uint64_t rdtsc();
inline uint64_t rdtscp();

inline int64_t time_t2hms(const time_t ct);
inline void split_hms(int hms, int& h, int& m, int& s);

/**
 * "23:59:59" to 235959
 */
int intraday_time_HMS(const char* str);
int intraday_time_HMS(const string& str);
int64_t usec_HMS(const char* str, int64_t ms = 0);
int64_t usec_HMS(const string& str, int64_t ms = 0);

/**
 * "23:59" to 2359
 */
int intraday_time_HM(const char* str);
int intraday_time_HM(const string& str);

/**
 * "20171112" to 20171112
 */
int cob(const char* str);
int cob(const string& str);

/**
 * "2017-11-12" to 20171112
 */
int cob_from_dash(const char* str);
int cob_from_dash(const string& str);

/**
 * "20171112 23:59:59" to time_t
 */
double time_string2double(const string& str);

uint64_t HumanReadableMicrosecond(const std::string& str);
uint64_t HumanReadableMicrosecond(const char* interval_string);
uint64_t HumanReadableMillisecond(const std::string& interval_string);
uint64_t HumanReadableMillisecond(const char* interval_string);

std::string replace_time_placeholder(const std::string& str, int d = 0, int32_t t = -1);

int GetNextDate(int cob);
std::vector<long> GenAllDaysInYear(long year);
}
