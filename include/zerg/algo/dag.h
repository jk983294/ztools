#pragma once

#include <algorithm>
#include <vector>

namespace zerg {
//! data structure denoting the entire dependency tree
template <typename T>
class DAG {
public:
    //! single node representing dependency item
    struct DependencyNode {
        long id;  // node index
        T* p;     // pointer pointing to real object.
        std::vector<size_t> dependencies;

        DependencyNode(long id_, T* p_) : id(id_), p(p_) {}
    };

    size_t ind{0};
    std::vector<DependencyNode*> tree;
    std::vector<size_t> sorted_list;

public:
    DAG() = default;

    ~DAG() {
        for (auto iter = tree.begin(); iter != tree.end(); ++iter) delete *iter;
    }

    size_t size() { return tree.size(); }

    bool empty() { return tree.empty(); }

    std::vector<size_t> getSortedList() { return sorted_list; }

    // add node to the dependency tree, return ind
    size_t AddNode(T* p) {
        auto node = new DependencyNode(static_cast<long>(ind), p);
        ++ind;
        tree.push_back(node);
        return ind - 1;
    }

    // add dependency
    int AddDependency(size_t dependent, size_t dependency) {
        if (dependent > ind || dependency > ind) return -1;
        tree[dependent]->dependencies.push_back(dependency);
        return 0;
    }

    // perform the topological sort
    void Sort() {
        std::vector<bool> explored(tree.size(), false);
        sorted_list.resize(tree.size(), 0);
        size_t t = tree.size();
        for (size_t i = 0; i < tree.size(); ++i) {
            if (explored[i] == false) {
                Sort_using_DFS(tree, explored, i, sorted_list, t);
            }
        }
        std::reverse(sorted_list.begin(), sorted_list.end());
    }

    void Sort_using_DFS(std::vector<DependencyNode*>& graph, std::vector<bool>& explored, size_t i,
                        std::vector<size_t>& sorted, size_t& t) {
        explored[i] = true;
        for (size_t j = 0; j < graph[i]->dependencies.size(); ++j) {
            if (explored[graph[i]->dependencies[j]] == false) {
                Sort_using_DFS(graph, explored, graph[i]->dependencies[j], sorted, t);
            }
        }
        --t;
        sorted[t] = i;
    }
};

}
