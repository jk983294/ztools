#pragma once

#include <cassert>
#include <cstdlib>
#include <vector>

namespace zerg {
constexpr size_t _POOL_PREPARE_TIMES = 5;
template <typename TData, size_t MEM_POOL_SIZE = 10000>
struct MemPool {
  struct node_t {
    struct node_t *m_next;
  };

  MemPool() {
    assert(sizeof(TData) >= sizeof(void *));
    m_barn.reserve(1000);
    std::vector<TData *> ptrs;
    ptrs.reserve(MEM_POOL_SIZE * _POOL_PREPARE_TIMES);
    for (size_t i = 0; i < MEM_POOL_SIZE * _POOL_PREPARE_TIMES; ++i) ptrs.push_back(allocate());
    for (auto *ptr : ptrs) deallocate(ptr);
  }

  ~MemPool() {
    for (auto ptr : m_barn) delete ptr;
    m_barn.clear();
  }

  inline TData *allocate() {
    if (m_cnt > 0) {
      return placement_new_data();
    } else if (m_free_list) {
      TData *ret = reinterpret_cast<TData *>(m_free_list);
      m_free_list = m_free_list->m_next;
      return ret;
    } else {
      alloc_pool();
      return placement_new_data();
    }
  }

  void deallocate(TData *data) {
    struct node_t *node = (struct node_t *)data;
    node->m_next = m_free_list;
    m_free_list = node;  // add to head
  }

  private:
  TData *placement_new_data() {
    TData *ret = m_curr->data() + (--m_cnt);
    return ret;
  }
  void alloc_pool() {
    m_curr = new std::vector<TData>(MEM_POOL_SIZE);
    m_cnt = MEM_POOL_SIZE;
    m_barn.push_back(m_curr);
  }

  size_t m_cnt = 0;
  node_t *m_free_list{nullptr};
  std::vector<TData> *m_curr{nullptr};
  std::vector<std::vector<TData> *> m_barn;
};

template <typename TPool>
struct PoolHolder {
  PoolHolder(int thread_num) {
    if (thread_num <= 0) thread_num = 1;
    pools.resize(thread_num);
    for (int i = 0; i < thread_num; ++i) {
      pools[i] = new TPool;
    }
  }
  ~PoolHolder() {
    for (auto *ptr : pools) {
      delete ptr;
    }
  }

  TPool *get_pool(int tid) { return pools[tid]; }

  std::vector<TPool *> pools;
};
}  // namespace zerg