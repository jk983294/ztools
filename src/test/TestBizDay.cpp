#include "catch.hpp"
#include "zerg/time/bizday.h"
#include <iostream>

using namespace std;
using namespace zerg;

TEST_CASE("BizDay prev_n function", "[bizday]") {
    std::string testFile = "/dat/ctp/trading_day.txt";
    BizDayConfig config(testFile);
    
    SECTION("prev_n with exact trading day, include_date=true") {
        auto result = config.prev_n(20080110, 3, true);
        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == 20080108);
        REQUIRE(result[1] == 20080109);
        REQUIRE(result[2] == 20080110);
    }
    
    SECTION("prev_n with exact trading day, include_date=false") {
        auto result = config.prev_n(20080110, 3, false);
        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == 20080107);
        REQUIRE(result[1] == 20080108);
        REQUIRE(result[2] == 20080109);
    }
    
    SECTION("prev_n with non-trading day (weekend), include_date=true") {
        auto result = config.prev_n(20080106, 2, true);  // 20080106 is Sunday
        REQUIRE(result.size() == 2);
        REQUIRE(result[0] == 20080103);
        REQUIRE(result[1] == 20080104);
    }
    
    SECTION("prev_n with non-trading day (weekend), include_date=false") {
        auto result = config.prev_n(20080106, 2, false);  // 20080106 is Sunday
        REQUIRE(result.size() == 2);
        REQUIRE(result[0] == 20080103);
        REQUIRE(result[1] == 20080104);
    }
    
    SECTION("prev_n with date before first trading day") {
        auto result = config.prev_n(20070101, 2, true);
        REQUIRE(result.empty());
    }
    
    SECTION("prev_n with n=1, include_date=true") {
        auto result = config.prev_n(20080115, 1, true);
        REQUIRE(result.size() == 1);
        REQUIRE(result[0] == 20080115);
    }
    
    SECTION("prev_n with n=1, include_date=false") {
        auto result = config.prev_n(20080115, 1, false);
        REQUIRE(result.size() == 1);
        REQUIRE(result[0] == 20080114);
    }
    
    SECTION("prev_n with early trading days, include_date=true") {
        auto result = config.prev_n(20080104, 3, true);
        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == 20080102);
        REQUIRE(result[1] == 20080103);
        REQUIRE(result[2] == 20080104);
    }
    
    SECTION("prev_n with early trading days, include_date=false") {
        auto result = config.prev_n(20080104, 3, false);
        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == 20080102);
        REQUIRE(result[1] == 20080103);
        REQUIRE(result[2] == 20080104);
    }
}
