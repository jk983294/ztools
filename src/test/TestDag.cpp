#include <iostream>
#include <string>
#include "catch.hpp"
#include "zerg/algo/dag.h"

using namespace std;

TEST_CASE("topo sort", "[DependencyTree]") {
    zerg::DAG<int> tree;

    auto ind1 = tree.AddNode(new int(0));
    auto ind2 = tree.AddNode(new int(1));
    auto ind3 = tree.AddNode(new int(2));
    auto ind4 = tree.AddNode(new int(3));
    auto ind5 = tree.AddNode(new int(4));
    auto ind6 = tree.AddNode(new int(5));
    auto ind7 = tree.AddNode(new int(6));

    tree.AddDependency(ind2, ind3);
    tree.AddDependency(ind3, ind1);
    tree.AddDependency(ind1, ind4);

    tree.Sort();
    std::vector<size_t> sorted = tree.getSortedList();

    REQUIRE(tree.size() == 7);

    std::vector<size_t> target{3, 0, 2, 1, 4, 5, 6};
    REQUIRE(sorted == target);
}
