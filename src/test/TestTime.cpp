#include "catch.hpp"
#include "zerg/time/time.h"
#include "zerg/time/clock.h"

using namespace std;
using namespace zerg;

TEST_CASE("GenAllDaysInYear", "[time]") {
    std::vector<int32_t> ret = zerg::GenAllDaysInYear(2018);
    REQUIRE(ret.front() == 20180101);
    REQUIRE(ret.back() == 20181231);
}

TEST_CASE("Constructor", "[time]") {
    auto clock = new Clock();
    clock->Update();
    REQUIRE(clock->DateToInt() >= 20180718);

    auto clock1 = new Clock("19:20:20", "%H:%M:%S");
    REQUIRE(clock1->TimeToInt() == 192020000);
}

TEST_CASE("Comparison", "[time]") {
    Clock clock1("19:20:20", "%H:%M:%S");
    Clock clock2("19:20:21", "%H:%M:%S");
    REQUIRE(clock2 > clock1);
    REQUIRE(clock2 >= clock1);

    Clock clock3("19:20:20", "%H:%M:%S");
    REQUIRE(clock3 == clock1);
}

TEST_CASE("Time trading clock", "[time]") {
    vector<int32_t> list = {20140530, 20140602, 20140603};
    Clock clock("20140601 19:20:20", "%Y%m%d %H:%M:%S", list);
    REQUIRE(clock.DateToInt() == 20140602);
}

TEST_CASE("Time trading clock late than 18:00", "[time]") {
    vector<int32_t> list = {20140530, 20140602, 20140603};
    Clock clock("20140602 19:20:20", "%Y%m%d %H:%M:%S", list);
    REQUIRE(clock.DateToInt() == 20140603);
}

TEST_CASE("Time trading clock late than 24:00", "[time]") {
    vector<int32_t> list = {20140530, 20140602, 20140603};
    Clock clockT("20140602 2:20:20", "%Y%m%d %H:%M:%S", list);
    REQUIRE(clockT.TimeToInt() == 262020000);
    REQUIRE(clockT.DateToInt() == 20140602);
}

TEST_CASE("Human Readable", "[time]") {
    auto ret = zerg::HumanReadableMillisecond("100ms");
    REQUIRE(ret == 100);

    ret = zerg::HumanReadableMillisecond("100milliseconds");
    REQUIRE(ret == 100);

    ret = zerg::HumanReadableMillisecond("100millisecond");
    REQUIRE(ret == 100);

    ret = zerg::HumanReadableMillisecond(" 10 min ");
    REQUIRE(ret == 600000);

    ret = zerg::HumanReadableMillisecond(" 10 minutes ");
    REQUIRE(ret == 600000);

    ret = zerg::HumanReadableMillisecond(" 10 minute ");
    REQUIRE(ret == 600000);

    ret = zerg::HumanReadableMicrosecond(" 10 us ");
    REQUIRE(ret == 10);

    ret = zerg::HumanReadableMicrosecond(" 10 macroseconds ");
    REQUIRE(ret == 10);

    ret = zerg::HumanReadableMicrosecond(" 10 macrosecond ");
    REQUIRE(ret == 10);

    ret = zerg::HumanReadableMicrosecond(" 10 sec ");
    REQUIRE(ret == 10 * 1000 * 1000);

    ret = zerg::HumanReadableMicrosecond(" 10 seconds ");
    REQUIRE(ret == 10 * 1000 * 1000);

    ret = zerg::HumanReadableMicrosecond(" 10 second ");
    REQUIRE(ret == 10 * 1000 * 1000);

    ret = zerg::HumanReadableMicrosecond(" 10 s ");
    REQUIRE(ret == 10 * 1000 * 1000);
}

TEST_CASE("Human Readable time", "[time]") {
    Clock clock1("21:01:12", "%H:%M:%S");
    std::string a(" 09:01:12 pm ");
    auto ret1 = zerg::HumanReadableTime(a);
    REQUIRE(ret1 == clock1);

    Clock clock2("09:01:12", "%H:%M:%S");
    std::string b(" 09:01:12 A.M.");
    auto ret2 = zerg::HumanReadableTime(b);
    REQUIRE(ret2 == clock2);

    Clock clock3("09:01:12", "%H:%M:%S");
    std::string c(" 09:01:12");
    auto ret3 = zerg::HumanReadableTime(c);
    REQUIRE(ret3 == clock3);

    Clock clock4("09:01", "%H:%M");
    std::string d(" 09:01");
    auto ret4 = zerg::HumanReadableTime(d);
    REQUIRE(ret4 == clock4);

    Clock clock5("21:01", "%H:%M");
    std::string e(" 09:01 PM");
    auto ret5 = zerg::HumanReadableTime(e);
    REQUIRE(ret5 == clock5);
}

TEST_CASE("GetNextDate", "[GetNextDate]") {
    REQUIRE(GetNextDate(20220929) == 20220930);
    REQUIRE(GetNextDate(20220930) == 20221001);
}
