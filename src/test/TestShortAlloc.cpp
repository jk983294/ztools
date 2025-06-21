#include <list>
#include <string>
#include <unordered_set>
#include "catch.hpp"
#include "short_alloc.h"

/**
 * Create a vector<T> template with a small buffer of 200 bytes.
 * Note for vector it is possible to reduce the alignment requirements down to alignof(T)
 * because vector doesn't allocate anything but T's. And if we're wrong about that guess,
 * it is a compile-time error, not a run time error.
 */
template <class T, std::size_t BufSize = 200>
using SmallVector = std::vector<T, short_alloc<T, BufSize, alignof(T)>>;

TEST_CASE("vector", "[short alloc]") {
    // Create the stack-based arena from which to allocate
    SmallVector<int>::allocator_type::arena_type a;
    // Create the vector which uses that arena.
    SmallVector<int> v{a};
    // Exercise the vector and note that new/delete are not getting called.
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    // Yes, the correct values are actually in the vector
    REQUIRE(v[0] == 1);
    REQUIRE(v[1] == 2);
    REQUIRE(v[2] == 3);
    REQUIRE(v[3] == 4);
}

template <class T, std::size_t BufSize = 200>
using SmallSet =
    std::unordered_set<T, std::hash<T>, std::equal_to<T>, short_alloc<T, BufSize, alignof(T) < 8 ? 8 : alignof(T)>>;

TEST_CASE("set", "[short alloc]") {
    SmallSet<int>::allocator_type::arena_type a;
    SmallSet<int> v{a};
    v.insert(1);
    v.insert(2);
    v.insert(3);
    v.insert(4);

    // Yes, the correct values are actually in the vector
    REQUIRE(v.find(1) != v.end());
    REQUIRE(v.find(2) != v.end());
    REQUIRE(v.find(3) != v.end());
    REQUIRE(v.find(4) != v.end());
    REQUIRE(v.find(5) == v.end());
}
