#pragma once
#include <cstdint>
#include <cstdio>
#include <optional>
#include <stdint.h>
#include <limits>
#include <cstdlib>
#include <types.hpp>

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
                                            std::memory_order_acquire,
                                            std::memory_order_relaxed))
            {
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
                                            std::memory_order_acquire,
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
    for (int i = 0; i < 1000; ++i) {
        LockFreeStackArray<int> stack(1'000'000);
        std::atomic<bool> start{false};
        constexpr int iter = 1'000'000;

        std::thread pusher1 = std::thread([&] {
            while (!start.load(std::memory_order_acquire)) {}

            for (int j = 0; j < iter; ++j) {
                while (!stack.push(j)) {
                    // Retry if stack is full
                    std::this_thread::yield();
                }
            }
        });

        std::thread pusher2 = std::thread([&] {
            while (!start.load(std::memory_order_acquire)) {}

            for (int j = 0; j < iter; ++j) {
                while (!stack.push(iter + j)) {
                    // Retry if stack is full
                    std::this_thread::yield();
                }
            }
        });

        std::thread popper1 = std::thread([&] {
            while (!start.load(std::memory_order_acquire)) {}

            for (int j = 0; j < iter; ++j) {
                while (!stack.pop()) {
                    // Retry if stack is empty
                    std::this_thread::yield();
                }
            }
        });

        std::thread popper2 = std::thread([&] {
            while (!start.load(std::memory_order_acquire)) {}

            for (int j = 0; j < iter; ++j) {
                while (!stack.pop()) {
                    // Retry if stack is empty
                    std::this_thread::yield();
                }
            }
        });

        start.store(true, std::memory_order_release);

        pusher1.join();
        pusher2.join();
        popper1.join();
        popper2.join();

        if (stack.size() > 0) {
            assert(0);
        }
        std::cout << "count: " << i << " " << stack.size() << "\n";
    }
}