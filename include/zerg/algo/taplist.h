#pragma once

#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>

namespace zerg {
// it is used to replace std::list
template <typename T, size_t InitSize = 1000, size_t InitBlock = 10>
class TapList {
public:
    // used as data block
    struct DataCell {
        T data{};
        DataCell* prev{nullptr};
        DataCell* next{nullptr};
    };

protected:
    // _head is used as iterator
    struct _head : std::iterator<std::bidirectional_iterator_tag, T> {
        DataCell* data{nullptr};
        DataCell* prev{nullptr};
        DataCell* next{nullptr};
        TapList* parent{nullptr};
        _head(TapList* tl) : parent(tl) {}
        _head(TapList* tl, DataCell* pData, DataCell* pPrev, DataCell* pNext)
            : data(pData), prev(pPrev), next(pNext), parent(tl) {}
        _head(TapList* tl, DataCell& dt) : data(&dt), prev(dt.prev), next(dt.next), parent(tl) {}

        T& operator*() { return data->data; }
        bool operator==(const _head& rhs) { return data == rhs.data; }
        bool operator!=(const _head& rhs) { return !operator==(rhs); }
        _head& operator++() {
            if (next == parent->tail.data) parent->extend();

            data = data->next;
            prev = data->prev;
            next = data->next;
            return *this;
        }
        _head& operator--() {
            if (prev == parent->head.data) parent->extend();

            data = data->prev;
            prev = data->prev;
            next = data->next;
            return *this;
        }

        _head operator++(int) {
            _head clone(*this);
            operator++();
            return clone;
        }

        _head operator--(int) {
            _head clone(*this);
            operator--();
            return clone;
        }
    };

public:
    typedef T value_type;
    typedef typename std::size_t size_type;
    typedef _head iterator;
    typedef _head const_iterator;
    typedef T& reference;
    typedef const T& const_reference;

protected:
    size_t m_size;          // size of each block
    size_t m_block_num{0};  // number of blocks
    _head head;
    _head tail;
    _head right_end;
    _head left_end;
    bool is_stolen{false};
    std::vector<DataCell*> vecs;
    std::vector<std::pair<DataCell*, DataCell*> > endpoints;
    size_t m_num{0};

public:
    void __clear() {
        for (size_t i = 0; i < vecs.size(); ++i) vecs[i] = nullptr;
    }

    iterator begin() { return tail; }
    iterator begin() const { return tail; }
    iterator rbegin() { return head; }
    iterator rbegin() const { return head; }
    iterator end() {
        if (!m_num && head == tail) return head;
        return step_forward(head);
    }

    iterator rend() {
        if (!m_num && head == tail) return head;
        return step_backward(tail);
    }

private:
    iterator step_forward(const iterator& it) {
        if (it.next == tail.data) extend();
        auto& dd = it.next;
        return _head(this, dd, dd->prev, dd->next);
    }

    iterator step_backward(const iterator& it) {
        if (it.prev == head.data) extend();
        auto& dd = it.prev;
        return _head(this, dd, dd->prev, dd->next);
    }

    void extend() {
        ++m_block_num;
        vecs.push_back(nullptr);
        auto& vec = vecs[m_block_num];
        vec = (DataCell*)malloc(m_size * sizeof(DataCell));
        PrepareVec(head.data, tail.data, vec);
        head.next = vec;
        head.data->next = head.next;
        tail.prev = vec + m_size;
        tail.data->prev = tail.prev;
    }

    void PrepareVec(DataCell* s, DataCell* e, DataCell* vec) {
        vec[0].next = &(vec[1]);
        if (s)
            vec[0].prev = s;
        else
            vec[0].prev = &(vec[m_size - 1]);

        for (size_t i = 1; i < m_size - 1; ++i) {
            vec[i].next = &(vec[i + 1]);
            vec[i].prev = &(vec[i - 1]);
        }

        vec[m_size - 1].prev = &(vec[m_size - 2]);
        if (e)
            vec[m_size - 1].next = e;
        else
            vec[m_size - 1].next = &(vec[0]);
    }

public:
    TapList() : head(this), tail(this), right_end(this), left_end(this) {
        vecs.reserve(InitBlock);
        vecs.push_back(nullptr);
        reserve(InitSize);
        m_size = InitSize;
        endpoints.reserve(InitBlock);
        endpoints.push_back(std::make_pair(vecs[0], vecs[0] + m_size));
        head = _head(this, *vecs[0]);
        tail = head;
    }

    virtual ~TapList() {
        if (!is_stolen && vecs[0]) free(vecs[0]);

        for (size_t i = 1; i < m_block_num; ++i) {
            if (vecs[i]) free(vecs[i]);
        }
    }

    void steal(DataCell* p, size_t size) {
        m_size = size;
        auto& vec = vecs[0];
        if (vec && !is_stolen) free(vec);

        vec = p;
        PrepareVec(nullptr, nullptr, vec);
        head = _head(this, *vec);
        tail = head;
        is_stolen = true;
    }

    void reserve(size_t size) {
        m_size = size;
        auto& vec = vecs[0];
        if (vec && !is_stolen) free(vec);
        vec = (DataCell*)malloc(m_size * sizeof(DataCell));
        PrepareVec(nullptr, nullptr, vec);
    }

    void clear() {
        head = _head(this, *vecs[0]);
        tail = head;
        m_num = 0;
    }

    void push_back(const value_type& val) {
        if (m_num) {
            if (head.next == tail.data) {
                // block is full, needs to extend
                extend();
            }
            ++head;
        }
        ++m_num;
        *head = val;
    }

    void push_front(const value_type& val) {
        if (m_num) {
            if (head.next == tail.data) {
                // block is full, needs to extend
                extend();
            }
            --tail;
        }
        ++m_num;
        *tail = val;
    }

    size_t size() { return m_num; }

    bool empty() { return (m_num == 0); }

    void pop_front() {
        if (!m_num) return;
        --m_num;
        if (tail != head) ++tail;
    }

    void erase_from_begin(iterator pos) {  // remove [begin, pos)
        for (; tail != pos; ++tail) --m_num;
        if (!m_num) head = tail;
    }

    void erase_from_end(iterator pos) {  // remove (pos, rbegin]
        for (; head != pos; --head) --m_num;
        if (!m_num) head = tail;
    }

    reference front() { return tail.data->data; }

    const_reference front() const { return tail.data->data; }

    reference back() { return head.data->data; }

    const_reference back() const { return head.data->data; }
};
}
