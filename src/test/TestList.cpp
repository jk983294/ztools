#include "catch.hpp"
#include "zerg/algo/taplist.h"

using namespace zerg;
using namespace std;

TEST_CASE("tap list", "[list]") {
    TapList<int, 5> tlist;
    tlist.push_back(1);
    tlist.push_back(2);
    REQUIRE(tlist.front() == 1);
    REQUIRE(tlist.back() == 2);
    REQUIRE(*(tlist.begin()) == 1);
    REQUIRE(*(++tlist.begin()) == 2);
    REQUIRE(*(tlist.rbegin()) == 2);
    REQUIRE(*(--tlist.rbegin()) == 1);

    tlist.push_front(3);
    REQUIRE(tlist.front() == 3);
    REQUIRE(tlist.size() == 3);
    REQUIRE(!tlist.empty());

    tlist.clear();
    tlist.push_back(3);
    tlist.push_front(4);
    REQUIRE(*(--tlist.rbegin()) == 4);
}

TEST_CASE("overflow", "[list]") {
    TapList<int, 3> tlist;
    tlist.push_back(1);
    tlist.push_back(2);
    tlist.push_back(3);
    tlist.push_back(4);
    tlist.push_back(5);
    tlist.push_back(6);
    tlist.pop_front();
    tlist.push_back(7);
    REQUIRE(tlist.front() == 2);
}

TEST_CASE("steal", "[list]") {
    TapList<int, 5> tlist;
    typedef TapList<int, 5> TL;
    TapList<int, 5>::DataCell* p = (TL::DataCell*)malloc(5 * sizeof(TL::DataCell));
    tlist.steal(p, 5);
    tlist.push_back(1);
    tlist.push_back(2);
    REQUIRE(*(--tlist.rbegin()) == 1);
}

TEST_CASE("steal overflow", "[list]") {
    TapList<int, 5> tlist;
    typedef TapList<int, 5> TL;
    TapList<int, 5>::DataCell* p = (TL::DataCell*)malloc(5 * sizeof(TL::DataCell));
    tlist.steal(p, 5);
    for (int i = 0; i < 6; ++i) {
        tlist.push_back(i);
    }
    REQUIRE(*(tlist.begin()) == 0);
    REQUIRE(*(tlist.rbegin()) == 5);
}

TEST_CASE("ERASEFROMBEGIN", "[list]") {
    TapList<int, 5> tlist;
    tlist.push_back(1);
    tlist.push_back(2);
    tlist.push_back(3);
    tlist.push_back(4);
    auto iter = tlist.rbegin();
    --iter;
    --iter;
    tlist.erase_from_begin(iter);

    REQUIRE((*tlist.begin()) == 2);
    REQUIRE(tlist.size() == 3);
}

TEST_CASE("ERASEFROMBEGINHEADEND", "[list]") {
    TapList<int, 5> tlist;

    tlist.push_back(1);
    auto iter = tlist.rbegin();

    tlist.erase_from_begin(iter);

    REQUIRE(tlist.size() == 1);
}

TEST_CASE("ERASEFROMEND", "[list]") {
    TapList<int, 5> tlist;

    tlist.push_back(1);
    tlist.push_back(2);
    tlist.push_back(3);
    tlist.push_back(4);
    auto iter = tlist.rbegin();
    --iter;
    --iter;

    tlist.erase_from_end(iter);

    REQUIRE((*tlist.rbegin()) == 2);
    REQUIRE(tlist.size() == 2);
}

TEST_CASE("POPFRONT", "[list]") {
    TapList<int, 5> tlist;

    tlist.push_back(1);
    tlist.push_back(2);
    tlist.push_back(3);

    tlist.pop_front();

    REQUIRE(tlist.size() == 2);
}

TEST_CASE("PopFrontSizeEqual", "[list]") {
    TapList<int, 5> tlist;

    tlist.push_back(1);

    tlist.pop_front();

    REQUIRE(tlist.size() == 0);
}
