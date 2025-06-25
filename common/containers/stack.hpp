#pragma once
#include <cstdint>
#include <cstdio>
#include <stdint.h>
#include <limits>
#include <cstdlib>
#include <types.hpp>

template<podtype T, std::unsigned_integral IndexType = uint16_t>
class Stack {
  public:
    Stack() 
        : capacity_(64), top_(-1) 
    {
        data_ = (T*)malloc(capacity_ * sizeof(T));
        assert(data_ && "malloc failed");
    };
    Stack(IndexType initialCapacity) 
        : capacity_(initialCapacity), top_(-1) 
    {
        data_ = (T*)malloc(initialCapacity * sizeof(T));
        assert(data_ && "malloc failed");
    }

    Stack(const Stack& other) = delete;
    Stack(Stack&& other) = delete;
    Stack& operator= (const Stack& other) = delete;
    Stack& operator=(Stack&& other) = delete;

    ~Stack() { if (data_) free(data_); }
    
    void push(const T& value) {
        if (full()) expand();
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

        data_ = static_cast<T*>(realloc(data_, newCapacity * sizeof(T)));
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
        data_ = (T*)realloc( data_, capacity_ * sizeof(T));
        assert(data_ && "realloc failed");
    }
};