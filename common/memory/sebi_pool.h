#pragma once

#include "types.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include <containers/stack.hpp>
#include <limits>
#include <memory>
#include <memoryapi.h>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <thread>
#include <type_traits>
#include <vector>

template<typename T, std::unsigned_integral IndexType = uint16_t>
struct Handle {
    IndexType index_;
    IndexType gen_;

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
            puts("[Pool::Recycle]: [InvalidHandle]/[UseAfterFree]: returning...");
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

template<typename T>
static constexpr T INVALID_INDEX_T = static_cast<T>(-1);
#define INVALID_INDEX_ INVALID_INDEX_T<IndexType>

template<std::integral IndexType = uint16_t>
class ThreadSafePoolStack
{
  public:
    static constexpr size_t max_capacity_allowed = std::numeric_limits<IndexType>::max() + 1;

    static constexpr size_t allocation_size()
    {
        return sizeof(uint64_t) * max_capacity_allowed;
    }
    
    ThreadSafePoolStack() = default;
    ThreadSafePoolStack(const ThreadSafePoolStack &other) = delete;
    ThreadSafePoolStack(ThreadSafePoolStack &&other) = delete;
    ThreadSafePoolStack &operator=(const ThreadSafePoolStack &other) = delete;
    ThreadSafePoolStack &operator=(ThreadSafePoolStack &&other) = delete;

    explicit ThreadSafePoolStack(uintptr_t base_addr, IndexType cap) noexcept
    {
        arr_ = reinterpret_cast<uint64_t *>(base_addr);
        for (IndexType i = 0; i < cap; ++i) {
            arr_[i] = (cap-1) - i;
        }
        top_.store(cap-1);
    }

    // insert a freeindex onto the stack.
    void push_multiple(uint64_t *arr, IndexType count)
    {
        for (IndexType t = top_.load()+1, i = 0; i < count; ++i, ++t) {
            arr_[t] = arr[i];
        }
        top_ += count;
    }

    // returns the index which is free inside the caller pool.
    [[nodiscard]]
    IndexType pop()
    {
        // this will cause a race. thread A goes inside the if check and before returning gets preempted, thread B
        // also goes inside the if-check, returns, stack expands cause of thread B, thread A resumes, returns and
        // causes another expand, you get two expansions.
        //
        /*
        if (top_.load(std::memory_order_relaxed) == INVALID_INDEX_) {
            return INVALID_INDEX_;
        }
        */

        IndexType i = top_.fetch_sub(1, std::memory_order_relaxed);
        return arr_[i];
    }

    // pool should be locked when calling this.
    void expand(IndexType old_capacity, IndexType new_capacity)
    {
        assert(!"should not be here!");
        assert(new_capacity > old_capacity);
        IndexType diff = new_capacity - old_capacity;
        for (IndexType i = 0, j = top_.load()+1; i < diff; ++i, ++j) {
            arr_[j] = (new_capacity-1) - i;
        }
        top_ += diff;
    }

    alignas(64) uint64_t *arr_ { nullptr };
    alignas(64) std::atomic<IndexType> top_ { INVALID_INDEX_ };
};

template <podtype T, std::unsigned_integral IndexType = uint16_t>
class ThreadSafePool
{
  public:
    static constexpr size_t allocation_size()
    {
        return (pool_arr_max_size    + generations_arr_max_size +
                deletes_arr_max_size + stack_max_size);
    }

    explicit ThreadSafePool(uintptr_t back_buffer, IndexType initial_capacity)
        : capacity_(initial_capacity),
          arr_(reinterpret_cast<T *>(back_buffer)),
          generations_(reinterpret_cast<uint64_t *>(back_buffer + generations_arr_offset)),
          batched_deletes_(reinterpret_cast<uint64_t *>(back_buffer + deletes_arr_offset)),
          freelist_(back_buffer + stack_offset, initial_capacity),
          batched_deletes_count_(0)
    {
        assert((uintptr_t)arr_ == back_buffer);
        assert((uintptr_t)generations_ == back_buffer + pool_arr_max_size);
        assert((uintptr_t)batched_deletes_ == back_buffer + pool_arr_max_size + generations_arr_max_size);
        assert((uintptr_t)freelist_.arr_ ==
               back_buffer + pool_arr_max_size + generations_arr_max_size + deletes_arr_max_size);
        assert(back_buffer + stack_offset + stack_max_size == back_buffer + allocation_size());
    }

    ThreadSafePool() = delete;
    ThreadSafePool(const ThreadSafePool& other) = delete;
    ThreadSafePool(ThreadSafePool&& other) = delete;
    ThreadSafePool& operator=(const ThreadSafePool& other) = delete;
    ThreadSafePool& operator=(ThreadSafePool&& other) = delete;

    [[nodiscard]]
    Handle<T> allocate()
    {
        std::shared_lock<std::shared_mutex> read_lock{ rw_mtx };
        IndexType free_index = freelist_.pop();
        if (free_index == INVALID_INDEX_) {
            // read_lock.unlock();
            // expand();
            // read_lock.lock();
            // free_index = freelist_.pop();
            assert(!"cannot expand without races for now!");
        }

        // num_allocations.fetch_add(1, std::memory_order_acq_rel);

        Handle<T, IndexType> handle;
        handle.index_ = free_index;
        handle.gen_ = generations_[free_index];
        return handle;
    }

    void recycle(const Handle<T> handle)
    {
        std::shared_lock<std::shared_mutex> read_lock{ rw_mtx };
        if (handle.gen_ != generations_[handle.index_]) {
            assert(handle.gen_ < generations_[handle.gen_]);
            puts("[LFPool::Recycle]: [InvalidHandle] returning...");
            return;
        }

        // deferring the frees until cleanup is called.
        IndexType batch_index = batched_deletes_count_.fetch_add(1, std::memory_order_relaxed);
        batched_deletes_[batch_index] = handle.index_;
    }

    [[nodiscard]]
    T *get(const Handle<T> handle) const noexcept
    {
        std::shared_lock<std::shared_mutex> read_lock{ rw_mtx };
        if (handle.gen_ != generations_[handle.index_]) {
            return nullptr;
        }

        return &arr_[handle.index_];
    }

    void cleanup()
    {
        std::lock_guard<std::shared_mutex> write_lock{ rw_mtx };
        IndexType batched_count = batched_deletes_count_;
        // printf("cleaning up pool: there are %d elements batched for deletion.\n", batched_count);
        freelist_.push_multiple(batched_deletes_, batched_count);
        for (int i = 0; i < batched_count; ++i) {
            ++generations_[batched_deletes_[i]];
        }

        // uint64_t alloc_num = num_allocations.load(std::memory_order_acquire);
        // assert(alloc_num >= batched_deletes_count_);
        // num_allocations.fetch_sub(batched_deletes_count_, std::memory_order_release);

        batched_deletes_count_ = 0;
    }

    /*
    uint64_t get_allocation_count() const {
        return num_allocations.load();
    }
    */

  private:
      /** not using this.
      void expand()
      {
          assert(!"should not be here!");
          std::lock_guard<std::shared_mutex> write_lock{rw_mtx};
          if (capacity_*2 >= max_capacity_allowed) {
              assert(!"Cannot expand this pool further!");
          }
          printf("[LFPool::Expand()]: expanding pool from %zu to %zu ...", capacity_, capacity_ * 2);

          freelist_.expand(capacity_, capacity_ * 2);
          capacity_ *= 2;
          puts(" ... done");
      }
      */
  private:
    alignas(64) T *arr_;
    alignas(64) uint64_t *generations_;
    alignas(64) uint64_t *batched_deletes_;
    alignas(64) size_t capacity_;
    // alignas(64) std::atomic<uint64_t> num_allocations;
    alignas(64) std::atomic<uint64_t> batched_deletes_count_;
    alignas(64) ThreadSafePoolStack<IndexType> freelist_;

    mutable std::shared_mutex rw_mtx;

  public:
    static constexpr size_t max_capacity_allowed = std::numeric_limits<IndexType>::max();

    static constexpr size_t pool_arr_max_size = sizeof(T) * max_capacity_allowed;
    static constexpr size_t generations_arr_max_size = sizeof(std::remove_pointer_t<decltype(generations_)>) *
                                                       max_capacity_allowed;
    static constexpr size_t deletes_arr_max_size = sizeof(std::remove_pointer_t<decltype(batched_deletes_)>) *
                                                   max_capacity_allowed;
    static constexpr size_t stack_max_size = ThreadSafePoolStack<IndexType>::allocation_size();

    static constexpr size_t pool_arr_offset = 0;
    static constexpr size_t generations_arr_offset = pool_arr_max_size;
    static constexpr size_t deletes_arr_offset = generations_arr_offset + generations_arr_max_size;
    static constexpr size_t stack_offset = deletes_arr_offset + deletes_arr_max_size;
};

#define POOL_UNIT_TESTS
#ifdef POOL_UNIT_TESTS
#include "memory.h"
#include <windows.h>
#include <winnt.h>

#include <chrono>
#include <gtest/gtest.h>

using namespace std::chrono_literals;
struct TestObject
{
    int id;
    int access_count;
    char padding[60];
};

using TestPool = ThreadSafePool<TestObject>;
class ThreadSafePoolTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);

        constexpr size_t alloc_size = TestPool::allocation_size();
        size_t sz = align_forward(alloc_size, sys_info.dwPageSize);
        back_buffer_ = VirtualAlloc((LPVOID)TERABYTES(4), sz, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        ASSERT_NE(back_buffer_, nullptr);

        pool_ = std::make_unique<TestPool>((uintptr_t)back_buffer_, initial_capacity_);

        cleanup_running_ = true;
        cleanup_thread_ = std::thread([this]() { cleanup_timer_loop(); });
    }

    void TearDown() override {
        cleanup_running_ = false;
        cleanup_cv_.notify_all();
        if (cleanup_thread_.joinable()) {
            cleanup_thread_.join();
        }
        if (back_buffer_) {
            VirtualFree(back_buffer_, 0, MEM_RELEASE);
        }
    }

    void cleanup_timer_loop() {
        std::unique_lock<std::mutex> lock{cleanup_mutex_};
        while(cleanup_running_) {
            cleanup_cv_.wait_for(lock, 64ms);
            if (cleanup_running_) {
                pool_->cleanup();
                cleanup_count_++;
            }
        }
    }

    void random_sleep(int min_us = 1, int max_us = 1000) {
        thread_local std::random_device rd;
        thread_local std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min_us, max_us);
        std::this_thread::sleep_for(std::chrono::microseconds(dis(gen)));
    }

  protected:
    static constexpr uint16_t initial_capacity_ = std::numeric_limits<uint16_t>::max();
    void *back_buffer_ = nullptr;
    std::unique_ptr<TestPool> pool_;

    std::atomic_bool cleanup_running_{false};
    std::thread cleanup_thread_;
    std::mutex cleanup_mutex_;
    std::condition_variable cleanup_cv_;
    std::atomic_int cleanup_count_{0};
};

TEST_F(ThreadSafePoolTest, BasicAllocationAndRecycle) {
    auto handle = pool_->allocate();
    EXPECT_NE(handle.index_, static_cast<uint16_t>(-1));

    TestObject *obj = pool_->get(handle);
    ASSERT_NE(obj, nullptr);
    obj->id = 42;

    pool_->recycle(handle);

    // Object should still be accessible until cleanup
    TestObject *obj2 = pool_->get(handle);
    EXPECT_NE(obj2, nullptr);
    EXPECT_EQ(obj2->id, 42);

    // spinlock until cleanup thread does not run.
    while (cleanup_count_ == 0) {
        std::this_thread::sleep_for(64ms);
    }

    // the handle should be invalid now since the thing it was pointing to inside the pool has been recycled by the
    // cleanup thread.
    TestObject *obj3 = pool_->get(handle);
    EXPECT_EQ(obj3, nullptr);
}

/** not doing expansion for now.
TEST_F(ThreadSafePoolTest, PoolExpansion) {
    std::vector<Handle<TestObject>> handles;
    for (int i = 0; i < initial_capacity_ * 3; ++i) {
        auto handle = pool_->allocate();
        handles.push_back(handle);

        TestObject *obj = pool_->get(handle);
        ASSERT_NE(obj, nullptr);
        obj->id = i;
    }

    for (size_t i = 0; i < handles.size(); ++i) {
        TestObject *obj = pool_->get(handles[i]);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(obj->id, static_cast<int>(i));
    }

    for (size_t i = 0; i < handles.size()/2; ++i) {
        pool_->recycle(handles[i]);
    }
}
*/

TEST_F(ThreadSafePoolTest, UseAfterFreeDetection) {
    auto handle = pool_->allocate();
    TestObject *obj = pool_->get(handle);
    ASSERT_NE(obj, nullptr);
    obj->id = 123;

    pool_->recycle(handle);

    // wait for the cleanup thread to do it's thing.
    while (cleanup_count_ == 0) {
        std::this_thread::sleep_for(32ms);
    }

    TestObject *invalid_obj = pool_->get(handle);
    EXPECT_EQ(invalid_obj, nullptr);

    auto new_handle = pool_->allocate();
    EXPECT_EQ(new_handle.index_, handle.index_);
    EXPECT_NE(new_handle.gen_, handle.gen_);

    EXPECT_EQ(pool_->get(handle), nullptr);
    EXPECT_NE(pool_->get(new_handle), nullptr);
}

TEST_F(ThreadSafePoolTest, ConcurrentAllocationStress) {
    const int num_threads = std::thread::hardware_concurrency();
    const size_t allocations_per_thread = (pool_->max_capacity_allowed / (size_t)num_threads);


    std::atomic_int total_allocations{0};
    std::atomic_int failed_allocations{0};
    std::vector<std::thread> threads;

    std::mutex handles_mutex;
    std::vector<Handle<TestObject>> global_handles;

    for (size_t thread_index = 0; thread_index < num_threads; ++thread_index) {
        threads.emplace_back([&, thread_index]() {
            thread_local std::vector<Handle<TestObject>> local_handles;
            for (int i = 0; i < allocations_per_thread; ++i) {
                random_sleep(1, 100);
                auto handle = pool_->allocate();
                TestObject *obj = pool_->get(handle);
                if (obj != nullptr) {
                    obj->id = thread_index * 1000 + i;
                    obj->access_count++;
                    local_handles.push_back(handle);
                    total_allocations.fetch_add(1);
                } else {
                    failed_allocations.fetch_add(1);
                }

                if (!local_handles.empty() && (i%10) == 0) {
                    auto idx = i % local_handles.size();
                    pool_->recycle(local_handles[idx]);
                    local_handles.erase(local_handles.begin() + idx);
                }
            }

            {
                std::lock_guard<std::mutex> lock{ handles_mutex };
                global_handles.insert(global_handles.end(), local_handles.begin(), local_handles.end());
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    printf("total allocations: %d.\n", total_allocations.load());
    printf("failed allocations: %d.\n", failed_allocations.load());
    printf("cleanup count: %d.\n", cleanup_count_.load());
    // printf("allocation count: %llu.\n", pool_->get_allocation_count());

    // no two handles should point to the same index inside the pool.
    std::vector<bool> temp(65536, false);
    for (const auto& handle : global_handles) {
        ASSERT_EQ(temp[handle.index_], false);
        temp[handle.index_] = true;
    }

    int valid_objects = 0;
    for (const auto& handle : global_handles) {
        TestObject *obj = pool_->get(handle);
        if (obj != nullptr) {
            valid_objects++;
            EXPECT_GT(obj->access_count, 0);
        }
    }

    // EXPECT_LE(pool_->get_allocation_count(), num_threads * allocations_per_thread);
    EXPECT_GT(valid_objects, 0);
    EXPECT_EQ(failed_allocations.load(), 0);
}

TEST_F(ThreadSafePoolTest, RecycleStorm)
{
    constexpr int num_objects = 100;
    const int num_threads = std::thread::hardware_concurrency();
    constexpr int cycles_per_thread = 500;

    std::vector<Handle<TestObject>> handles;
    for (int i = 0; i < num_objects; ++i) {
        handles.push_back(pool_->allocate());
    }

    std::atomic_int recycle_count{ 0 };
    std::atomic_int invalid_handle_count{ 0 };
    std::vector<std::thread> threads;

    for (int thread_index = 0; thread_index < num_threads; ++thread_index) {
        threads.emplace_back([&]() {
            thread_local std::random_device rd;
            thread_local std::mt19937 gen(rd());
            for (int i=0; i<cycles_per_thread; ++i) {
                random_sleep(1, 50);
                auto index = std::uniform_int_distribution<size_t>(0, handles.size() - 1)(gen);
                auto handle = handles[index];

                TestObject *obj = pool_->get(handle);
                if (obj != nullptr) {
                    obj->access_count++;
                    if (i%3 == 0) {
                        pool_->recycle(handle);
                        recycle_count.fetch_add(1);
                    }
                } else {
                    invalid_handle_count.fetch_add(1);
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    printf("Recycles: %d.\n", recycle_count.load());
    printf("Invalid handles encountered: %d.\n", invalid_handle_count.load());
    printf("cleanup cycles: %d.\n", cleanup_count_.load());

    EXPECT_GT(recycle_count.load(), 0);
    EXPECT_GT(cleanup_count_.load(), 0);
}

TEST_F(ThreadSafePoolTest, RandomStress) {
    const int num_threads = std::thread::hardware_concurrency();
    constexpr int operations_per_thread = 2000;
    std::atomic_bool test_running{true};

    std::mutex handles_mutex;
    std::vector<Handle<TestObject>> shared_handles;

    std::atomic_int allocate_ops{ 0 };
    std::atomic_int recycle_ops{ 0 };
    std::atomic_int get_ops{ 0 };
    std::atomic_int null_gets{ 0 };

    std::vector<std::thread> threads;
    for (int thread_index = 0; thread_index < num_threads; ++thread_index) {
        threads.emplace_back([&, thread_index]() {
            thread_local std::random_device rd;
            thread_local std::mt19937 gen(rd());
            thread_local std::vector<Handle<TestObject>> local_handles;
            for (int i = 0; i < operations_per_thread && test_running; ++i) {
                random_sleep(1, 20);
                int op = std::uniform_int_distribution<int>(0, 2)(gen);
                switch (op)
                {
                    case 0:
                    {
                        auto handle = pool_->allocate();
                        TestObject *obj = pool_->get(handle);
                        if (obj) {
                            obj->id = thread_index * 1000 + i;
                            local_handles.push_back(handle);
                            allocate_ops.fetch_add(1);
                        }
                    } break;

                    case 1:
                    {
                        if (!local_handles.empty()) {
                            auto index = std::uniform_int_distribution<int>(0, local_handles.size() - 1)(gen);
                            pool_->recycle(local_handles[index]);
                            local_handles.erase(local_handles.begin() + index);
                            recycle_ops.fetch_add(1);
                        }
                    } break;

                    case 2:
                    {
                        Handle<TestObject> test_handle{ };
                        {
                            std::lock_guard<std::mutex> lock(handles_mutex);
                            if (!shared_handles.empty()) {
                                auto index = std::uniform_int_distribution<int>(0, shared_handles.size() - 1)(gen);
                                test_handle = shared_handles[index];
                            } else if (!local_handles.empty()) {
                                auto index = std::uniform_int_distribution<int>(0, local_handles.size() - 1)(gen);
                                test_handle = local_handles[index];
                            }
                        }
                        if (test_handle.index_ != static_cast<uint16_t>(-1)) {
                            TestObject *obj = pool_->get(test_handle);
                            if (obj) {
                                obj->access_count++;
                            } else {
                                null_gets.fetch_add(1);
                            }
                            get_ops.fetch_add(1);
                        }
                    } break;
                }

            }

            {
                std::lock_guard<std::mutex> lock(handles_mutex);
                shared_handles.insert(shared_handles.end(), local_handles.begin(), local_handles.end());
            }
        });
    }

    std::this_thread::sleep_for(2000ms);
    test_running = false;

    for (auto &thread : threads) {
        thread.join();
    }

    puts("random stress test results:");
    printf("allocations: %d.\n", allocate_ops.load());
    printf("recycles: %d.\n", recycle_ops.load());
    printf("gets: %d.\n", get_ops.load());
    printf("null gets: %d.\n", null_gets.load());
    printf("cleanup cycles: %d.\n", cleanup_count_.load());
    printf("shared handles: %zu.\n", shared_handles.size());

    EXPECT_GT(allocate_ops.load(), 0);
    EXPECT_GT(cleanup_count_.load(), 0);
}
#endif // POOL_UNIT_TESTS