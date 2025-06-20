
#pragma once

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

namespace zerg {

/**
 * vector based set
 * iterators are invalidated by insert and erase operations
 * the complexity of insert/erase is O(N)
 * iterators are random
 */
template <class K, class C = std::less<K>, class A = std::allocator<K> >
class ZergSet : private std::vector<K> {
    typedef std::vector<K> Base;
    C cmp;

public:
    typedef K key_type;
    typedef typename Base::value_type value_type;

    typedef C key_compare;
    typedef K mapped_type;
    typedef A allocator_type;
    typedef typename A::reference reference;
    typedef typename A::const_reference const_reference;
    typedef typename Base::iterator iterator;
    typedef typename Base::const_iterator const_iterator;
    typedef typename Base::size_type size_type;
    typedef typename Base::difference_type difference_type;
    typedef typename A::pointer pointer;
    typedef typename A::const_pointer const_pointer;
    typedef typename Base::reverse_iterator reverse_iterator;
    typedef typename Base::const_reverse_iterator const_reverse_iterator;

    class value_compare : public std::binary_function<value_type, value_type, bool> {
        friend class ZergSet;

    protected:
        value_compare(key_compare pred) : key_compare(pred) {}

    public:
        bool operator()(const value_type& lhs, const value_type& rhs) const {
            return key_compare::operator()(lhs, rhs);
        }
    };

    explicit ZergSet(const key_compare& comp = key_compare(), const A& alloc = A()) : Base(alloc), cmp(comp) {}

    template <class InputIterator>
    ZergSet(InputIterator first, InputIterator last, const key_compare& comp = key_compare(), const A& alloc = A())
        : Base(first, last, alloc), cmp(comp) {
        std::sort(begin(), end(), cmp);
    }

    ZergSet& operator=(const ZergSet& rhs) {
        ZergSet(rhs).swap(*this);
        return *this;
    }

    iterator begin() { return Base::begin(); }
    const_iterator begin() const { return Base::begin(); }
    iterator end() { return Base::end(); }
    const_iterator end() const { return Base::end(); }
    reverse_iterator rbegin() { return Base::rbegin(); }
    const_reverse_iterator rbegin() const { return Base::rbegin(); }
    reverse_iterator rend() { return Base::rend(); }
    const_reverse_iterator rend() const { return Base::rend(); }

    bool empty() const { return Base::empty(); }
    size_type size() const { return Base::size(); }
    size_type max_size() { return Base::max_size(); }

    mapped_type& operator[](const key_type& key) { return *(insert(value_type(key, mapped_type())).first); }

    mapped_type& operator[](int ind) { return Base::operator[](ind); }

    std::pair<iterator, bool> insert(const value_type& val) {
        bool found(true);
        iterator i(lower_bound(val));

        if (i == end() || cmp(val, *i)) {
            i = Base::insert(i, val);
            found = false;
        }
        return std::make_pair(i, !found);
    }

    iterator insert(iterator pos, const value_type& val) {
        if (pos != end() && cmp(*pos, val) && ((pos == end() - 1) || (!cmp(val, pos[1]) && cmp(pos[1], val)))) {
            return Base::insert(pos, val);
        }
        return insert(val).first;
    }

    template <class InputIterator>
    void insert(InputIterator first, InputIterator last) {
        for (; first != last; ++first) insert(*first);
    }

    void erase(iterator pos) { Base::erase(pos); }

    size_type erase(const key_type& k) {
        iterator i(find(k));
        if (i == end()) return 0;
        erase(i);
        return 1;
    }

    void erase(iterator first, iterator last) { Base::erase(first, last); }

    void swap(ZergSet& other) {
        using std::swap;
        Base::swap(other);
    }

    void clear() { Base::clear(); }

    iterator find(const key_type& k) {
        iterator i(lower_bound(k));
        if (i != end() && cmp(k, *i)) {
            i = end();
        }
        return i;
    }

    const_iterator find(const key_type& k) const {
        const_iterator i(lower_bound(k));
        if (i != end() && cmp(k, *i)) {
            i = end();
        }
        return i;
    }

    size_type count(const key_type& k) const { return find(k) != end(); }

    iterator lower_bound(const key_type& k) { return std::lower_bound(begin(), end(), k, cmp); }

    const_iterator lower_bound(const key_type& k) const { return std::lower_bound(begin(), end(), k, cmp); }

    iterator upper_bound(const key_type& k) { return std::upper_bound(begin(), end(), k, cmp); }

    const_iterator upper_bound(const key_type& k) const { return std::upper_bound(begin(), end(), k, cmp); }

    std::pair<iterator, iterator> equal_range(const key_type& k) { return std::equal_range(begin(), end(), k, cmp); }

    std::pair<const_iterator, const_iterator> equal_range(const key_type& k) const {
        return std::equal_range(begin(), end(), k, cmp);
    }

    friend bool operator==(const ZergSet& lhs, const ZergSet& rhs) {
        const Base& me = lhs;
        return me == rhs;
    }

    bool operator<(const ZergSet& rhs) const {
        const Base& me = *this;
        const Base& yo = rhs;
        return me < yo;
    }

    friend bool operator!=(const ZergSet& lhs, const ZergSet& rhs) { return !(lhs == rhs); }

    friend bool operator>(const ZergSet& lhs, const ZergSet& rhs) { return rhs < lhs; }

    friend bool operator>=(const ZergSet& lhs, const ZergSet& rhs) { return !(lhs < rhs); }

    friend bool operator<=(const ZergSet& lhs, const ZergSet& rhs) { return !(rhs < lhs); }
};

// specialized algorithms:
template <class K, class C, class A>
void swap(ZergSet<K, C, A>& lhs, ZergSet<K, C, A>& rhs) {
    lhs.swap(rhs);
}

}
