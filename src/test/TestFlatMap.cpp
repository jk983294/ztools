#include "FlatMap.h"
#include "catch.hpp"

using namespace std;

TEST_CASE("flat map", "[flat map]") {
    FlatMap<std::string, int> map;
    map["one"] = 1;
    REQUIRE(map.size() == 1);

    bool find = map.find("one") != map.end();
    REQUIRE(find);

    find = map.find("two") != map.end();
    REQUIRE(!find);

    map["two"] = 2;
    int sum = 0;

    for (auto iter = map.begin(); iter != map.end(); ++iter) {
        sum += iter->second;
    }
    REQUIRE(sum == 3);
}
