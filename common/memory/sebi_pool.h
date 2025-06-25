#pragma once

#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include <containers/stack.hpp>

template<typename T>
struct Handle {
    uint16_t index_;
    uint16_t gen_;
};

template<podtype T>
class Pool {
  public:
    Pool() : numAllocations_(0),
             capacity_(64)
    {
        arr_  = static_cast<T*>( malloc(capacity_ * sizeof(T)) );
        generations_ = static_cast<uint16_t *>( calloc(capacity_, sizeof(uint16_t)) );
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

    void recycle(const Handle<T>& handle) {
        assert(handle.gen_ == generations_[handle.index_]);
        freelist_.push(handle.index_);
        if (generations_[handle.index_] + 1 < generations_[handle.index_]) {
            assert(!"overflow!");
        }
        ++generations_[handle.index_];
        --numAllocations_;
    }

    [[nodiscard]]
    T* get(const Handle<T>& handle) const noexcept {
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
        for (int i=oldCapacity; i<capacity_; ++i) {
            freelist_.push_safe(i);
        }
    }

  private:
    T *arr_;
    uint16_t *generations_;
    Stack<uint16_t> freelist_;

    uint16_t capacity_;
    uint16_t numAllocations_;
};