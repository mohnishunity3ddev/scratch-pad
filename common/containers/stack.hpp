#pragma once
#include <cstdint>
#include <cstdio>
#include <functional>
#include <optional>
#include <stdint.h>
#include <limits>
#include <cstdlib>
#include <types.hpp>
#include <vector>

template<podtype T, std::unsigned_integral IndexType = uint16_t>
class Stack {
  public:
    Stack() : capacity_(64), top_(-1)
    {
        data_ = static_cast<T*>( malloc(capacity_ * sizeof(T)) );
        assert(data_ && "malloc failed");
    };
    Stack(IndexType initialCapacity) : capacity_(initialCapacity), top_(-1)
    {
        data_ = static_cast<T*>( malloc(initialCapacity * sizeof(T)) );
        assert(data_ && "malloc failed");
    }

    Stack(const Stack& other) = delete;
    Stack(Stack&& other) = delete;
    Stack& operator= (const Stack& other) = delete;
    Stack& operator=(Stack&& other) = delete;

    ~Stack()
    {
        if (data_) {
            free(data_);
        }
    }

    void push(const T& value) {
        if (full()) {
            expand();
        }
        data_[++top_] = value;
    }

    void push_safe(const T& value) {
        data_[++top_] = value;
    }

    T pop() {
        if (empty()) {
            printf("stack empty!");
            return T{};
        }
        return data_[top_--];
    }

    void reserve(IndexType newCapacity) {
        if (newCapacity <= capacity_) {
            return;
        }

        data_ = static_cast<T*>( realloc(data_, newCapacity * sizeof(T)) );
        capacity_ = newCapacity;
    }

    [[nodiscard]]
    inline bool full() const noexcept {
        return ((uint16_t)(top_+1) >= capacity_);
    }
    [[nodiscard]]
    inline bool empty() const noexcept { return top_ == -1; }

  private:
    T* data_ = nullptr;
    IndexType top_, capacity_;

    void expand() {
        constexpr IndexType maxCap = std::numeric_limits<IndexType>::max() - 1;

        if (capacity_ >= maxCap/2) {
            capacity_ = maxCap;
        } else {
            capacity_ *= 2;
        }
        data_ = static_cast<T*>( realloc( data_, capacity_ * sizeof(T)) );
        assert(data_ && "realloc failed");
    }
};

#include <atomic>
#include <iostream>
#include <thread>
#include <cassert>

template<podtype T>
class LockFreeStackArray
{
  public:
    explicit LockFreeStackArray(size_t cap = DEFAULT_CAPACITY) : capacity_(cap), state_(0)
    {
        data_ = static_cast<T*>( malloc(capacity_ * sizeof(T)) );
    }

    ~LockFreeStackArray() {
        if (data_) {
            free(data_);
        }
    }

    LockFreeStackArray(const LockFreeStackArray&) = delete;
    LockFreeStackArray& operator=(const LockFreeStackArray&) = delete;

    bool push(const T& item)
    {
        uint64_t current_state = state_.load(std::memory_order_acquire);

        for (;;) {
            if (current_state & PENDING_BIT) {
                std::this_thread::yield();
                current_state = state_.load(std::memory_order_acquire);
                continue;
            }

            uint64_t count = current_state & COUNT_MASK;

            if (count >= capacity_) {
                return false;
            }

            uint64_t new_state = (count + 1) | PENDING_BIT;
            if (state_.compare_exchange_weak(current_state, new_state,
                                            std::memory_order_relaxed,
                                            std::memory_order_relaxed))
            {
                // This write is protected: No pop will read from this slot while the pending bit is set.
                data_[count] = item;
                state_.store(count + 1, std::memory_order_release);
                return true;
            }
        }
    }

    std::optional<T> pop()
    {
        uint64_t current_state = state_.load(std::memory_order_acquire);

        for (;;) {
            if (current_state & PENDING_BIT) {
                std::this_thread::yield();
                current_state = state_.load(std::memory_order_acquire);
                continue;
            }

            uint64_t count = current_state & COUNT_MASK;
            if (count == 0) {
                return std::nullopt;
            }

            // set pending bit
            uint64_t new_state = (count - 1) | PENDING_BIT;
            if (state_.compare_exchange_weak(current_state, new_state,
                                            std::memory_order_relaxed,
                                            std::memory_order_relaxed))
            {
                T result = data_[count - 1];

                // unset pending bit
                state_.store(count - 1, std::memory_order_release);
                return result;
            }
        }
    }

    size_t size() const {
        uint64_t current_state = state_.load(std::memory_order_acquire);
        return current_state & COUNT_MASK;
    }

    bool empty() const {
        return size() == 0;
    }

    bool full() const {
        return size() >= capacity_;
    }

    size_t get_capacity() const {
        return capacity_;
    }

  private:
    static constexpr size_t DEFAULT_CAPACITY = 1000000;

    T *data_;
    size_t capacity_;
    // A single atomic that encodes both the count and a "pending" bit
    // Lower 63 bits: stack_size, MSB: indicates if a push is in progress
    std::atomic<uint64_t> state_;
    
    static constexpr uint64_t PENDING_BIT = 1ULL << 63;
    static constexpr uint64_t COUNT_MASK = ~PENDING_BIT;
};

void lockfree_stack_test() {
    const int thread_count = std::thread::hardware_concurrency();
    const int pusher_count = thread_count / 2;
    const int popper_count = thread_count - pusher_count;
    printf("thread_count: %d, pusher_count: %d, popper_count: %d\n", thread_count, pusher_count, popper_count);
    constexpr int num_iterations = 1'000'000;
    const size_t stack_capacity = pusher_count * num_iterations;

    for (int i = 0; i < 1000; ++i) {
        LockFreeStackArray<int> stack(stack_capacity);
        std::atomic<bool> start{false};
        std::vector<std::thread> pushers;
        std::vector<std::thread> poppers;

        for (int i = 0; i < pusher_count; ++i) {
            pushers.emplace_back([&, i] {
                while (!start.load(std::memory_order_acquire)) {}

                for (int j = 0; j < num_iterations; ++j) {
                    while (!stack.push(j)) {
                        // Retry if stack is full
                        std::this_thread::yield();
                    }
                    // size_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
                    // printf("pusher[%zu] pushed %d.\n", tid, j);
                }
            });
        }

        for (int i = 0; i < popper_count; ++i) {
            poppers.emplace_back([&, i] {
                while (!start.load(std::memory_order_acquire)) {}

                for (int j = 0; j < num_iterations; ++j) {
                    std::optional<int> result;

                    while (!stack.pop()) {
                        // Retry if stack is empty
                        std::this_thread::yield();
                    }
                    // size_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
                    // printf("popper[%zu] popped.\n", tid);
                }
            });
        }

        start.store(true, std::memory_order_release);

        for (auto& pusher : pushers) {
            pusher.join();
        }
        for (auto& popper : poppers) {
            popper.join();
        }

        if (stack.size() > 0) {
            assert(0);
        }
        std::cout << "count: " << i << " " << stack.size() << "\n";
    }
}