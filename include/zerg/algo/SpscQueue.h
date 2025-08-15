#pragma once

#include <atomic>
#include <stdexcept>
#include <type_traits>

namespace zerg {
template <typename T>
class SpscQueue {
private:
    static constexpr size_t CacheLineSize = 64;
    // one cache line contains #PaddingCountOfT elements of T
    static constexpr size_t PaddingCountOfT = (CacheLineSize - 1) / sizeof(T) + 1;

private:
    const size_t capacity_;
    T *const slots_;

    // Align to avoid false sharing between headIndex_ and tailIndex_
    alignas(CacheLineSize) std::atomic<size_t> headIndex_;
    alignas(CacheLineSize) std::atomic<size_t> tailIndex_;

    // Padding to avoid adjacent allocations to share cache line with tailIndex_
    char padding_[CacheLineSize - sizeof(tailIndex_)];

public:
    explicit SpscQueue(const size_t capacity)
        : capacity_(capacity),
          slots_(static_cast<T *>(operator new[](sizeof(T) * (capacity_ + 2 * PaddingCountOfT)))),
          headIndex_(0),
          tailIndex_(0) {
        if (capacity_ < 2) throw std::invalid_argument("size < 2");
    }

    ~SpscQueue() {
        while (front()) {
            pop();
        }
        operator delete[](slots_);
    }

    // non-copyable and non-movable
    SpscQueue(const SpscQueue &) = delete;
    SpscQueue &operator=(const SpscQueue &) = delete;

    template <typename... Args>
    void emplace(Args &&... args) noexcept(std::is_nothrow_constructible<T, Args &&...>::value) {
        static_assert(std::is_constructible<T, Args &&...>::value, "T must be constructible with Args&&...");
        auto const head = headIndex_.load(std::memory_order_relaxed);
        auto nextHead = head + 1;
        if (nextHead == capacity_) {
            nextHead = 0;
        }

        while (nextHead == tailIndex_.load(std::memory_order_acquire))
            ;  // no space to push element in, then while loop until there is some space

        new (&slots_[head + PaddingCountOfT]) T(std::forward<Args>(args)...);
        headIndex_.store(nextHead, std::memory_order_release);
    }

    template <typename... Args>
    bool try_emplace(Args &&... args) noexcept(std::is_nothrow_constructible<T, Args &&...>::value) {
        static_assert(std::is_constructible<T, Args &&...>::value, "T must be constructible with Args&&...");
        auto const head = headIndex_.load(std::memory_order_relaxed);
        auto nextHead = head + 1;
        if (nextHead == capacity_) {
            nextHead = 0;
        }
        if (nextHead == tailIndex_.load(std::memory_order_acquire)) {
            return false;  // if no space to push, then return false
        }
        new (&slots_[head + PaddingCountOfT]) T(std::forward<Args>(args)...);
        headIndex_.store(nextHead, std::memory_order_release);
        return true;
    }

    void push(const T &v) noexcept(std::is_nothrow_copy_constructible<T>::value) {
        static_assert(std::is_copy_constructible<T>::value, "T must be copy constructible");
        emplace(v);
    }

    template <typename P, typename = typename std::enable_if<std::is_constructible<T, P &&>::value>::type>
    void push(P &&v) noexcept(std::is_nothrow_constructible<T, P &&>::value) {
        emplace(std::forward<P>(v));
    }

    bool try_push(const T &v) noexcept(std::is_nothrow_copy_constructible<T>::value) {
        static_assert(std::is_copy_constructible<T>::value, "T must be copy constructible");
        return try_emplace(v);
    }

    template <typename P, typename = typename std::enable_if<std::is_constructible<T, P &&>::value>::type>
    bool try_push(P &&v) noexcept(std::is_nothrow_constructible<T, P &&>::value) {
        return try_emplace(std::forward<P>(v));
    }

    /**
     * check if there is element ready for consume
     * @return nullptr means no elements to consume
     */
    T *front() noexcept {
        auto const tail = tailIndex_.load(std::memory_order_relaxed);
        if (headIndex_.load(std::memory_order_acquire) == tail) {
            return nullptr;
        }
        return &slots_[tail + PaddingCountOfT];
    }

    /**
     * this call should follow size(), make sure it must have item to consume
     */
    T *front_no_check() noexcept {
        auto const tail = tailIndex_.load(std::memory_order_relaxed);
        return &slots_[tail + PaddingCountOfT];
    }

    void pop() noexcept {
        static_assert(std::is_nothrow_destructible<T>::value, "T must be nothrow destructible");
        auto const tail = tailIndex_.load(std::memory_order_relaxed);

        slots_[tail + PaddingCountOfT].~T();
        auto nextTail = tail + 1;
        if (nextTail == capacity_) {
            nextTail = 0;
        }
        tailIndex_.store(nextTail, std::memory_order_release);
    }

    size_t size() const noexcept {
        ssize_t diff = static_cast<ssize_t>(headIndex_.load(std::memory_order_acquire)) -
                       static_cast<ssize_t>(tailIndex_.load(std::memory_order_acquire));
        if (diff < 0) {
            diff += static_cast<ssize_t>(capacity_);
        }
        return static_cast<size_t>(diff);
    }

    bool empty() const noexcept { return size() == 0; }

    size_t capacity() const noexcept { return capacity_; }

    /**
     * this call must cooperate with advance_head
     * @return
     */
    T& get_cell(size_t& nextHead) noexcept {
        auto const head = headIndex_.load(std::memory_order_relaxed);
        nextHead = head + 1;
        if (nextHead == capacity_) {
            nextHead = 0;
        }

        while (nextHead == tailIndex_.load(std::memory_order_acquire))
            ;

        return slots_[head + PaddingCountOfT];
    }

    /**
     * this call must follow get_cell(), basically get_cell() get space to produce
     * after produce, you need to call advance_head() to notify consumer to consume
     */
    void advance_head(size_t nextHead){
        headIndex_.store(nextHead, std::memory_order_release);
    }
};
} 

