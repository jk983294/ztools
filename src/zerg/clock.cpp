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

/**
 * example:
 * 12:30
 * 1:39:30
 * 1:39:29.300
 * 1:39:19 am (AM, a.m. A.M.)
 * @param literal
 * @return
 */
Clock<> HumanReadableTime(const std::string& literal) {
    std::string hour = "0";
    std::string min = "00";
    std::string sec = "00";
    std::string ms = "000";
    bool is_pm = false;

    std::string time_string = literal.c_str();
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
        min = time.substr(found + 1, time.size() - found - 1).c_str();

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
    auto clock = Clock<>(lit.c_str(), "%H:%M:%S");
    clock.nanoSecond = std::stoi(ms) * 1000 * 1000;
    clock.core.tv_nsec = clock.nanoSecond;
    return clock;
}

}
