#pragma once

#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include <containers/stack.hpp>
#include <shared_mutex>

template<typename T>
struct Handle {
    uint16_t index_;
    uint16_t gen_;

    bool operator!=(const Handle<T> &other) const {
        return index_ != other.index_ || gen_ != other.gen_;
    }

    bool operator==(const Handle<T> &other) const {
        return index_ == other.index_ || gen_ == other.gen_;
    }
};

template<podtype T>
class Pool {
  public:
    Pool() : capacity_(64)
    {
        arr_  = static_cast<T*>( malloc(capacity_ * sizeof(T)) );
        generations_ = static_cast<uint16_t *>( calloc(capacity_, sizeof(uint16_t)) );
        // TODO: Bulk fill stack and set top thereafter.
        for (int i = 0; i < 64; ++i) {
            freelist_.push_safe(63-i);
        }
    }

    Pool(const Pool& other) = delete;
    Pool(Pool&& other) = delete;
    Pool& operator= (const Pool& other) = delete;
    Pool& operator= (Pool&& other) = delete;

    [[nodiscard]]
    Handle<T> allocate() {
        if ((numAllocations_+1) >= capacity_) {
            expand();
        }
        ++numAllocations_;

        Handle<T> handle;
        handle.index_ = freelist_.pop();
        handle.gen_ = generations_[handle.index_];
        return handle;
    }

    void recycle(const Handle<T> handle) {
        if (handle.gen_ != generations_[handle.index_]) {
            printf("[InvalidHandle]/[UseAfterFree]: returning...\n");
            return;
        }
        freelist_.push(handle.index_);
        assert(generations_[handle.index_] + 1 > generations_[handle.index_] && "Overflow condition!");
        ++generations_[handle.index_];
        --numAllocations_;
    }

    [[nodiscard]]
    T* get(const Handle<T> handle) const noexcept {
        if (handle.gen_ != generations_[handle.index_]) {
            return nullptr;
        }
        return &arr_[handle.index_];
    }

    ~Pool() {
        if (arr_) { free(arr_); }
        if (generations_) { free(generations_); }
    }


  private:
    void expand() {
        uint16_t oldCapacity = capacity_;
        capacity_ *= 2;
        generations_ = static_cast<uint16_t *>( realloc(generations_, capacity_ * sizeof(uint16_t)) );
        memset(generations_ + oldCapacity, 0, (capacity_ - oldCapacity) * sizeof(uint16_t));
        arr_ = static_cast<T*>( realloc(arr_, capacity_ * sizeof(T)) );

        freelist_.reserve(capacity_);
        // TODO: Bulk fill stack and set top thereafter.
        for (int i = oldCapacity; i < capacity_; ++i) {
            freelist_.push_safe(i);
        }
    }

  private:
    T *arr_{ nullptr };
    uint16_t *generations_{ nullptr };
    Stack<uint16_t> freelist_;

    uint16_t capacity_ {0};
    uint16_t numAllocations_ {0};
};

// FIXME: This suffers from false sharing. Consider sharding. the ThreadSafeHandle gets a shard index. shard array
// contains arr_, generations_ etc. when allocating =, shard_index is atomically incremented in a round robin
// fashion to get the shard for the ThreadHandle. make sure the shard struct is atleast equal to the
// CACHE_LINE_SIZE (64 bytes) to avoid cache ping-pong.
template<podtype T>
class ThreadSafePool {
  public:
    ThreadSafePool(const ThreadSafePool& other) = delete;
    ThreadSafePool(ThreadSafePool&& other) = delete;
    ThreadSafePool& operator= (const ThreadSafePool& other) = delete;
    ThreadSafePool& operator= (ThreadSafePool&& other) = delete;

    [[nodiscard]]
    Handle<T> allocate() {
        std::lock_guard<std::shared_mutex> lock{m};
        return pool_.allocate();
    }

    void recycle(const Handle<T>& handle) {
        std::lock_guard<std::shared_mutex> lock{ m };
        pool_.recycle(handle);
    }

    [[nodiscard]]
    T *get(const Handle<T> &handle) const noexcept
    {
        std::shared_lock lock{ m };
        return pool_.get(handle);
    }

  private:
    Pool<T> pool_{};
    mutable std::shared_mutex m;
};