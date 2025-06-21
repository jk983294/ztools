#include <iostream>
#include "catch.hpp"
#include "zerg/time/clock.h"

TEST_CASE("replace time holder", "[utils]") {
    zerg::Clock clock;
    clock.Set(100);
    std::string str = "${YYYY}-${MM}-${DD}-${HHMMSSmmm}.xml";
    auto str2 = str;
    zerg::ReplaceSpecialTimeHolder(str2);
    std::cout << "default to current timestamp" << str2 << std::endl;
    ReplaceSpecialTimeHolder(str, clock);
    REQUIRE(str == "1970-01-01-080140000.xml");
}

TEST_CASE("replace time holder with string time", "[utils]") {
    zerg::Clock clock;
    clock.Set(100);
    std::string str = "${YYYY}-${MM}-${DD}-${HHMMSSmmm}.xml";
    zerg::ReplaceSpecialTimeHolder(str, "2018/02/03 08:23:40", "%Y/%m/%d %H:%M:%S");
    REQUIRE(str == "2018-02-03-082340000.xml");
}

TEST_CASE("replace string holder", "[utils]") {
    std::map<std::string, std::string> string_holders;
    string_holders["path"] = "aa/debug";
    string_holders["lib"] = "aa/lib";
    std::string str = "1-${path}-${lib}-1.xml";
    zerg::ReplaceStringHolder(str, string_holders);
    REQUIRE(str == "1-aa/debug-aa/lib-1.xml");
}
