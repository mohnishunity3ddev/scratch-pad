#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"
#include "memory/memory.h"

#define QUEUE_API(T, name)                                                                                        \
    typedef struct queue_##name                                                                                   \
    {                                                                                                             \
        const alloc_api *api;                                                                                     \
        T *arr;                                                                                                   \
        size_t capacity;                                                                                          \
        size_t front;                                                                                             \
        size_t length;                                                                                            \
    } queue_##name;                                                                                               \
                                                                                                                  \
    queue_##name queue_##name##_create(const alloc_api *api);                                                     \
    queue_##name queue_##name##_create_cap(const alloc_api *api, size_t initial_capacity);                        \
    void queue_##name##_display(const queue_##name *q);                                                           \
    void queue_##name##_push(queue_##name *q, T e);                                                               \
    T queue_##name##_pop(queue_##name *q);                                                                        \
    T queue_##name##_peek(const queue_##name *q, size_t i);                                                       \
    void queue_##name##_clear(queue_##name *q);                                                                   \
    void queue_##name##_free(queue_##name *q);
#define QUEUE_API_DEFAULT(T) QUEUE_API(T, T)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QUEUE_API(void *, voidp)
QUEUE_API_DEFAULT(int)

#define qinit(api,t) queue_##t##_create(api)
#define qpush(q,t,v) queue_##t##_push(q,v)
#define qpop(q,t)    queue_##t##_pop(q)
#define qpeek(q,t,i) queue_##t##_peek(q,i)
#define qclear(q,t)  queue_##t##_clear(q)
#define qfree(q,t)   queue_##t##_free(q);

#define qinit_p(api) queue_voidp_create(api)
#define qpush_p(q,v) queue_voidp_push(q,(void*)v)
#define qpop_p(q,t) (t*)queue_voidp_pop(q)
#define qpeek_p(q,t,i) (t*)queue_voidp_peek(q,i)
#define qclear_p(q) queue_voidp_clear(q)
#define qfree_p(q) queue_voidp_free(q);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#ifdef QUEUE_IMPLEMENTATION
#define QUEUE_API_IMPL(T, name)                                                                                   \
    static void queue_##name##_expand(queue_##name *q)                                                            \
    {                                                                                                             \
        assert(q != NULL);                                                                                        \
        q->capacity *= 2;                                                                                         \
        q->arr = shrealloc_arr(q->api, q->arr, T, q->capacity);                                                   \
        if (q->front > 0)                                                                                         \
        {                                                                                                         \
            /* the circular buffer is full and has wrapped around. */                                             \
            for (size_t i = 0; i < q->front; ++i)                                                                 \
            {                                                                                                     \
                /* adding elements wrapped around the old buffer after front. */                                  \
                q->arr[q->length + i] = q->arr[i];                                                                \
            }                                                                                                     \
            for (size_t i = 0; i < q->length; ++i)                                                                \
            {                                                                                                     \
                q->arr[i] = q->arr[q->front + i];                                                                 \
            }                                                                                                     \
            q->front = 0;                                                                                         \
        }                                                                                                         \
        assert(q->arr != NULL);                                                                                   \
    }                                                                                                             \
                                                                                                                  \
    queue_##name queue_##name##_create_cap(const alloc_api *api, size_t initial_capacity)                         \
    {                                                                                                             \
        queue_##name q;                                                                                           \
        q.api = api;                                                                                              \
        q.capacity = initial_capacity;                                                                            \
        q.arr = shalloc_arr(q.api, T, q.capacity);                                                                \
        q.length = 0;                                                                                             \
        q.front = -1;                                                                                             \
        return q;                                                                                                 \
    }                                                                                                             \
                                                                                                                  \
    queue_##name queue_##name##_create(const alloc_api *api) { return queue_##name##_create_cap(api, 2); }        \
                                                                                                                  \
    T queue_##name##_peek(const queue_##name *q, size_t i)                                                        \
    {                                                                                                             \
        if (i < 0 || i >= q->length)                                                                              \
        {                                                                                                         \
            printf("Index out of bounds!\n");                                                                     \
            return (T){0};                                                                                        \
        }                                                                                                         \
                                                                                                                  \
        assert(is_power_of_2(q->capacity));                                                                       \
        /* since q is a power of 2, we can use this instead of mod (%) */                                         \
        size_t index = (q->front + i) % q->capacity;                                                              \
        return q->arr[index];                                                                                     \
    }                                                                                                             \
                                                                                                                  \
    void queue_##name##_display(const queue_##name *q)                                                            \
    {                                                                                                             \
        if (q->length == 0)                                                                                       \
        {                                                                                                         \
            printf("Queue is empty!\n");                                                                          \
            return;                                                                                               \
        }                                                                                                         \
                                                                                                                  \
        printf("Queue [Length:= %zu].\n", q->length);                                                             \
        printf("FRONT -> ");                                                                                      \
        size_t index = q->front;                                                                                  \
        for (size_t i = 0; i < q->length; ++i)                                                                    \
        {                                                                                                         \
            printf(GET_FORMAT_STR(q->arr[0]), queue_##name##_peek(q, i));                                         \
        }                                                                                                         \
                                                                                                                  \
        printf("\b\b <- END\n");                                                                                  \
    }                                                                                                             \
                                                                                                                  \
    void queue_##name##_push(queue_##name *q, T e)                                                                \
    {                                                                                                             \
        if (q->length == 0)                                                                                       \
        {                                                                                                         \
            q->front = 0;                                                                                         \
        }                                                                                                         \
                                                                                                                  \
        if (q->length == q->capacity)                                                                             \
        {                                                                                                         \
            queue_##name##_expand(q);                                                                             \
        }                                                                                                         \
                                                                                                                  \
        assert(is_power_of_2(q->capacity));                                                                       \
        size_t index = (q->front + q->length) % q->capacity;                                                      \
        q->arr[index] = e;                                                                                        \
        ++q->length;                                                                                              \
    }                                                                                                             \
                                                                                                                  \
    T queue_##name##_pop(queue_##name *q)                                                                         \
    {                                                                                                             \
        assert(q != NULL);                                                                                        \
        if (q->length == 0)                                                                                       \
        {                                                                                                         \
            printf("queue is empty!\n");                                                                          \
            return (T){0};                                                                                        \
        }                                                                                                         \
                                                                                                                  \
        T result = q->arr[q->front];                                                                              \
        assert(is_power_of_2(q->capacity));                                                                       \
        q->front = (q->front + 1) % q->capacity;                                                                  \
        --q->length;                                                                                              \
                                                                                                                  \
        return result;                                                                                            \
    }                                                                                                             \
                                                                                                                  \
    void queue_##name##_clear(queue_##name *q)                                                                    \
    {                                                                                                             \
        q->length = 0;                                                                                            \
        q->front = -1;                                                                                            \
    }                                                                                                             \
    void queue_##name##_free(queue_##name *q)                                                                     \
    {                                                                                                             \
        queue_##name##_clear(q);                                                                                  \
        shfree(q->api, q->arr);                                                                                   \
    }

#define QUEUE_API_IMPL_DEFAULT(T) QUEUE_API_IMPL(T, T)
QUEUE_API_IMPL(void *, voidp)
QUEUE_API_IMPL_DEFAULT(int)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#ifdef QUEUE_UNIT_TESTS
void queue_unit_tests();

#ifndef FREELIST_ALLOCATOR_IMPLEMENTATION
#define FREELIST_ALLOCATOR_IMPLEMENTATION
#endif // FREELIST_ALLOCATOR_IMPLEMENTATION
#include "memory/freelist_alloc.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

// Basic enqueue and dequeue
static void
test_queue_basic_operations()
{
    Freelist fl;
    freelist_init(&fl, malloc(1024 * 1024), 1024 * 1024, DEFAULT_ALIGNMENT);
    fl.policy = PLACEMENT_POLICY_FIND_BEST;

    queue_int q = qinit(&fl.api, int);
    qpush(&q, int, 10);
    qpush(&q, int, 20);
    qpush(&q, int, 30);

    assert(qpop(&q, int) == 10);
    assert(qpop(&q, int) == 20);
    assert(qpop(&q, int) == 30);

    assert(q.length == 0);
    free(fl.data);
}

// Overflow test
static void
test_queue_overflow()
{
    Freelist fl;
    freelist_init(&fl, malloc(1024 * 1024), 1024 * 1024, DEFAULT_ALIGNMENT);
    fl.policy = PLACEMENT_POLICY_FIND_BEST;
    queue_int q = qinit(&fl.api, int);
    qpush(&q, int, 1);
    qpush(&q, int, 2);
    qpush(&q, int, 3);
    qpush(&q, int, 4);
    // Attempting to push beyond capacity should expand the queue or handle overflow gracefully
    qpush(&q, int, 5);
    assert(q.capacity >= 5);
    free(fl.data);
}

// Underflow test
static void
test_queue_underflow()
{
    Freelist fl;
    freelist_init(&fl, malloc(1024 * 1024), 1024 * 1024, DEFAULT_ALIGNMENT);
    fl.policy = PLACEMENT_POLICY_FIND_BEST;
    queue_int q = qinit(&fl.api, int);
    qpush(&q, int, 1);
    qpop(&q, int);
    assert(q.length == 0);
    // Attempting to pop from an empty queue should be handled properly
    int result = qpop(&q, int); // Modify implementation to handle this appropriately (e.g., return -1 or some
                                    // error code)
    // Replace with appropriate assertion if pop handles empty cases differently
    assert(result == 0);
    free(fl.data);
}

// Push and pop sequence with capacity increase
static void
test_queue_push_pop_sequence()
{
    Freelist fl;
    freelist_init(&fl, malloc(1024 * 1024), 1024 * 1024, DEFAULT_ALIGNMENT);
    fl.policy = PLACEMENT_POLICY_FIND_BEST;
    queue_int q = qinit(&fl.api, int);
    // Push and pop repeatedly to ensure the queue maintains the correct order and capacity management
    for (int i = 1; i <= 10; ++i)
    {
        qpush(&q, int, i);
        assert(qpop(&q, int) == i);
    }
    assert(q.length == 0);
    free(fl.data);
}

// Wraparound test (for circular buffer behavior)
static void
test_queue_wraparound()
{
    Freelist fl;
    freelist_init(&fl, malloc(1024 * 1024), 1024 * 1024, DEFAULT_ALIGNMENT);
    fl.policy = PLACEMENT_POLICY_FIND_BEST;
    queue_int q = qinit(&fl.api, int);
    qpush(&q, int, 1);
    qpush(&q, int, 2);
    qpush(&q, int, 3);
    qpush(&q, int, 4);
    qpop(&q, int);
    qpush(&q, int, 5);
    // Check if the queue maintains the correct order after wraparound
    assert(qpop(&q, int) == 2);
    assert(qpop(&q, int) == 3);
    assert(qpop(&q, int) == 4);
    assert(qpop(&q, int) == 5);
    assert(q.length == 0);
    free(fl.data);
}

static void
test_queue_large_number_of_operations()
{
    Freelist fl;
    freelist_init(&fl, malloc(1024 * 1024), 1024 * 1024, DEFAULT_ALIGNMENT);
    queue_int q = qinit(&fl.api, int);
    for (int i = 0; i < 100000; ++i)
    {
        qpush(&q, int, i);
        assert(q.length == (size_t)(i + 1));
    }
    for (int i = 0; i < 100000; ++i)
    {
        assert(qpop(&q, int) == i);
    }
    assert(q.length == 0);
    free(fl.data);
}

static void
test_queue_capacity_expansion()
{
    Freelist fl;
    freelist_init(&fl, malloc(1024 * 1024), 1024 * 1024, DEFAULT_ALIGNMENT);
    queue_int q = qinit(&fl.api, int);
    for (int i = 0; i < 32; ++i) {
        qpush(&q, int, i);
    }
    assert(q.capacity >= 32);
    for (int i = 0; i < 32; ++i) {
        assert(qpop(&q, int) == i);
    }
    assert(q.length == 0);
    free(fl.data);
}

static void
test_queue_alternate_push_pop()
{
    Freelist fl;
    freelist_init(&fl, malloc(1024 * 1024), 1024 * 1024, DEFAULT_ALIGNMENT);
    queue_int q = qinit(&fl.api, int);
    for (int i = 0; i < 1000; ++i) {
        qpush(&q, int, i);
        assert(qpop(&q, int) == i);
        assert(q.length == 0);
    }
    free(fl.data);
}

static void
test_queue_memory_integrity()
{
    Freelist fl;
    freelist_init(&fl, malloc(1024 * 1024), 1024 * 1024, DEFAULT_ALIGNMENT);
    queue_int q = qinit(&fl.api, int);
    qpush(&q, int, 1);
    qpush(&q, int, 2);
    qpush(&q, int, 3);
    qpush(&q, int, 4);
    qpush(&q, int, 5);
    qpush(&q, int, 6);
    qpop(&q, int); qpop(&q, int); qpop(&q, int);
    qpush(&q, int, 7);
    qpush(&q, int, 8);
    qpush(&q, int, 9);
    qpush(&q, int, 10);
    qpush(&q, int, 11);
    // trigger realloc with queue in a wrapped around state.
    qpush(&q, int, 12);
    qpush(&q, int, 13);
    qpush(&q, int, 14);
    qpush(&q, int, 15);
    qpush(&q, int, 16);
    qpush(&q, int, 17);
    qpush(&q, int, 18);
    qpush(&q, int, 19);
    qpush(&q, int, 20);
    qpop(&q, int); qpop(&q, int); qpop(&q, int);
    qpush(&q, int, 21);
    qpush(&q, int, 22);
    qpush(&q, int, 24);
    qpush(&q, int, 25);
    qpush(&q, int, 26);
    qpush(&q, int, 27);
    qpush(&q, int, 28);
    qpush(&q, int, 29);
    qpush(&q, int, 30);
    qpush(&q, int, 31);
    qpush(&q, int, 32);
    qpush(&q, int, 33);
    qpush(&q, int, 34);
    qpush(&q, int, 35);
    qpush(&q, int, 36);
    qpush(&q, int, 37);
    qpush(&q, int, 38);
    qpush(&q, int, 39);
    qpush(&q, int, 40);
    qpush(&q, int, 41);
    queue_int_display(&q);
    // clear queue for next tests.
    queue_int_clear(&q);
    for (int i = 0; i < 500; ++i) {
        qpush(&q, int, i);
        if (i % 2 == 0)
            qpop(&q, int);
    }
    for (int i = 500; i < 1000; ++i)
    {
        qpush(&q, int, i);
    }
    for (int i = 250; i < 1000; ++i)
    {
        assert(qpop(&q, int) == i);
    }
    assert(q.length == 0);
    free(fl.data);
}

static void
test_queue_boundary_conditions()
{
    Freelist fl;
    freelist_init(&fl, malloc(1024 * 1024), 1024 * 1024, DEFAULT_ALIGNMENT);
    queue_int q = qinit(&fl.api, int);
    qpush(&q, int, 1);
    assert(q.length == 1);
    assert(qpop(&q, int) == 1);
    assert(q.length == 0);
    qpush(&q, int, 2);
    assert(q.length == 1);
    assert(qpop(&q, int) == 2);
    assert(q.length == 0);
    qpush(&q, int, 3);
    qpush(&q, int, 4);
    assert(q.capacity >= 2);
    assert(qpop(&q, int) == 3);
    assert(qpop(&q, int) == 4);
    assert(q.length == 0);
    free(fl.data);
}

void
queue_unit_tests()
{
    test_queue_basic_operations();
    test_queue_overflow();
    test_queue_underflow();
    test_queue_push_pop_sequence();
    test_queue_wraparound();
    test_queue_large_number_of_operations();
    test_queue_capacity_expansion();
    test_queue_alternate_push_pop();
    test_queue_memory_integrity();
    test_queue_boundary_conditions();
    printf("All tests passed.\n");
}

#endif // QUEUE_UNIT_TESTS
#endif // QUEUE_IMPLEMENTATION
#endif // QUEUE_H