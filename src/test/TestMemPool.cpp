#include "catch.hpp"
#include "zerg/tool/mem_pool.h"

using namespace zerg;
using namespace std;

struct TestData {
  int64_t a;
  int64_t b;
};

TEST_CASE("mem pool basic allocation", "[mem pool]") {
    MemPool<TestData, 10> pool;

    TestData* data = pool.allocate();
    REQUIRE(data != nullptr);

    data->a = 42;
    data->b = 100;
    REQUIRE(data->a == 42);
    REQUIRE(data->b == 100);

    pool.deallocate(data);
}

TEST_CASE("mem pool multiple allocations", "[mem pool]") {
    MemPool<TestData, 5> pool;

    vector<TestData*> ptrs;
    for (int i = 0; i < 20; ++i) {
        TestData* data = pool.allocate();
        REQUIRE(data != nullptr);
        data->a = i;
        data->b = i * 2;
        ptrs.push_back(data);
    }

    for (int i = 0; i < 20; ++i) {
        REQUIRE(ptrs[i]->a == i);
        REQUIRE(ptrs[i]->b == i * 2);
    }

    for (auto* ptr : ptrs) {
        pool.deallocate(ptr);
    }
}

TEST_CASE("mem pool deallocate and reuse", "[mem pool]") {
    MemPool<TestData, 5> pool;

    TestData* ptr1 = pool.allocate();
    ptr1->a = 100;
    ptr1->b = 200;

    pool.deallocate(ptr1);

    TestData* ptr2 = pool.allocate();
    REQUIRE(ptr2 == ptr1);  // Should get the same pointer from free list
    REQUIRE(ptr2->a == 100);  // Data should still be there (not reset)
    REQUIRE(ptr2->b == 200);

    pool.deallocate(ptr2);
}

TEST_CASE("mem pool free list order", "[mem pool]") {
    MemPool<TestData, 3> pool;

    TestData* ptr1 = pool.allocate();
    TestData* ptr2 = pool.allocate();
    TestData* ptr3 = pool.allocate();

    pool.deallocate(ptr1);
    pool.deallocate(ptr2);
    pool.deallocate(ptr3);

    TestData* reuse1 = pool.allocate();
    TestData* reuse2 = pool.allocate();
    TestData* reuse3 = pool.allocate();

    // Should get from free list in reverse order (LIFO)
    REQUIRE(reuse1 == ptr3);
    REQUIRE(reuse2 == ptr2);
    REQUIRE(reuse3 == ptr1);

    pool.deallocate(reuse1);
    pool.deallocate(reuse2);
    pool.deallocate(reuse3);
}

TEST_CASE("mem pool exhaust current chunk", "[mem pool]") {
    MemPool<TestData, 5> pool;

    vector<TestData*> ptrs;
    for (int i = 0; i < 15; ++i) {
        TestData* data = pool.allocate();
        REQUIRE(data != nullptr);
        data->a = i;
        ptrs.push_back(data);
    }

    for (int i = 0; i < 15; ++i) {
        REQUIRE(ptrs[i]->a == i);
    }

    for (auto* ptr : ptrs) {
        pool.deallocate(ptr);
    }
}

TEST_CASE("mem pool allocate deallocate cycle", "[mem pool]") {
    MemPool<TestData, 3> pool;

    vector<TestData*> ptrs;

    // Allocate 10 items
    for (int i = 0; i < 10; ++i) {
        TestData* data = pool.allocate();
        data->a = i;
        data->b = i * 10;
        ptrs.push_back(data);
    }

    // Deallocate first 5
    for (int i = 0; i < 5; ++i) {
        pool.deallocate(ptrs[i]);
    }

    // Allocate 5 more, should reuse from free list
    for (int i = 0; i < 5; ++i) {
        TestData* data = pool.allocate();
        data->a = 100 + i;
        data->b = (100 + i) * 10;
        ptrs.push_back(data);
    }

    // Clean up all
    for (auto* ptr : ptrs) {
        pool.deallocate(ptr);
    }
}

TEST_CASE("mem pool PoolHolder basic", "[mem pool]") {
    PoolHolder<MemPool<TestData, 10>> holder(3);

    for (int i = 0; i < 3; ++i) {
        auto* pool = holder.get_pool(i);
        REQUIRE(pool != nullptr);

        TestData* data = pool->allocate();
        REQUIRE(data != nullptr);
        data->a = i;
        data->b = i * 10;

        REQUIRE(data->a == i);
        REQUIRE(data->b == i * 10);

        pool->deallocate(data);
    }
}

TEST_CASE("mem pool PoolHolder independent pools", "[mem pool]") {
    PoolHolder<MemPool<TestData, 5>> holder(2);

    auto* pool0 = holder.get_pool(0);
    auto* pool1 = holder.get_pool(1);

    // Allocate from pool0
    TestData* ptr0 = pool0->allocate();
    ptr0->a = 100;

    // Allocate from pool1
    TestData* ptr1 = pool1->allocate();
    ptr1->a = 200;

    REQUIRE(ptr0->a == 100);
    REQUIRE(ptr1->a == 200);

    pool0->deallocate(ptr0);
    pool1->deallocate(ptr1);
}

TEST_CASE("mem pool TestData size requirement", "[mem pool]") {
    // This should compile since TestData (16 bytes) >= sizeof(void*)
    MemPool<TestData, 10> pool;

    TestData* data = pool.allocate();
    REQUIRE(data != nullptr);
    pool.deallocate(data);
}
