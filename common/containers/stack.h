#ifndef STACK_H
#define STACK_H

#include "common.h"
#include "memory/memory.h"
#include <stdio.h>

#define STACK_API(T, name)                                                                                        \
    typedef struct stack_##name                                                                                   \
    {                                                                                                             \
        const alloc_api *api;                                                                                     \
        T *arr;                                                                                                   \
        size_t capacity;                                                                                          \
        signed long long top;                                                                                     \
    } stack_##name;                                                                                               \
    stack_##name stack_##name##_create_cap(const alloc_api *api, size_t initial_capacity);                        \
    stack_##name stack_##name##_create(const alloc_api *api);                                                     \
    void stack_##name##_push(stack_##name *s, T element);                                                         \
    T stack_##name##_pop(stack_##name *s);                                                                        \
    inline bool stack_##name##_empty(const stack_##name *s);                                                      \
    inline void stack_##name##_clear(stack_##name *s);                                                            \
    T stack_##name##_peek(stack_##name *s, size_t index);
#define STACK_API_DEFAULT(T) STACK_API(T, T)
STACK_API(void*, voidp)
STACK_API_DEFAULT(int)

#define sinit(api,t) stack_##t##_create(api)
#define spush(s,t,v) stack_##t##_push(&s,(v))
#define spop(s,t)    stack_##t##_pop(&s)
#define speek(s,t,i) stack_##t##_peek(&s,(i))
#define sempty(s,t)  stack_##t##_empty(&s)
#define sclear(s,t)  stack_##t##_clear(&s)

#define sinit_p(api) stack_voidp_create(api)
#define spush_p(s,v) stack_voidp_push(&s,(void*)v)
#define spop_p(s,t) (t*)stack_voidp_pop(&s)
#define speek_p(s,t,i) (t*)stack_voidp_peek(&s,i)
#define sempty_p(s) stack_voidp_empty(&s)
#define sclear_p(s) stack_voidp_clear(&s)

#define STACK_API_IMPL(T, name)                                                                                   \
    stack_##name stack_##name##_create_cap(const alloc_api *api, size_t initial_capacity)                         \
    {                                                                                                             \
        stack_##name s;                                                                                           \
        s.api = api;                                                                                              \
        s.capacity = initial_capacity;                                                                            \
        s.arr = shalloc_arr(s.api, T, s.capacity);                                                                \
        s.top = -1;                                                                                               \
        return s;                                                                                                 \
    }                                                                                                             \
                                                                                                                  \
    stack_##name stack_##name##_create(const alloc_api *api) { return stack_##name##_create_cap(api, 8); }        \
                                                                                                                  \
    inline bool stack_##name##_empty(const stack_##name *s) { return (s->top == -1); }                            \
                                                                                                                  \
    static inline bool stack_##name##_full(const stack_##name *s) { return ((s->top + 1) >= s->capacity); }       \
                                                                                                                  \
    static void stack_##name##_expand(stack_##name *s)                                                            \
    {                                                                                                             \
        s->capacity *= 2;                                                                                         \
        s->arr = shrealloc_arr(s->api, s->arr, T, s->capacity);                                                   \
    }                                                                                                             \
                                                                                                                  \
    void stack_##name##_push(stack_##name *s, T element)                                                          \
    {                                                                                                             \
        if (stack_##name##_full(s))                                                                               \
        {                                                                                                         \
            stack_##name##_expand(s);                                                                             \
        }                                                                                                         \
        s->arr[++s->top] = element;                                                                               \
    }                                                                                                             \
                                                                                                                  \
    T stack_##name##_pop(stack_##name *s)                                                                         \
    {                                                                                                             \
        if (stack_##name##_empty(s))                                                                              \
        {                                                                                                         \
            return (T){0};                                                                                        \
        }                                                                                                         \
        T result = s->arr[s->top];                                                                                \
        memset(s->arr + s->top, -842150451, sizeof(T));                                                           \
        s->top--;                                                                                                 \
        return result;                                                                                            \
    }                                                                                                             \
                                                                                                                  \
    inline void stack_##name##_clear(stack_##name *s) { s->top = -1; }                                            \
    T stack_##name##_peek(stack_##name *s, size_t index)                                                          \
    {                                                                                                             \
        if (index < 0 || index > s->top)                                                                          \
        {                                                                                                         \
            printf("index out of bounds.\n");                                                                     \
            return (T){0};                                                                                        \
        }                                                                                                         \
        T result = s->arr[s->top - index];                                                                        \
        return result;                                                                                            \
    }
#define STACK_API_IMPL_DEFAULT(T) STACK_API_IMPL(T, T)

#ifdef STACK_IMPLEMENTATION
STACK_API_IMPL(void*, voidp)
STACK_API_IMPL_DEFAULT(int)

#ifdef STACK_UNIT_TESTS
#ifndef FREELIST_ALLOCATOR_IMPLEMENTATION
#define FREELIST_ALLOCATOR_IMPLEMENTATION
#endif
#include "memory/freelist_alloc.h"
#include <time.h>
STACK_API_DEFAULT(int)
STACK_API_IMPL_DEFAULT(int)

void
stack_unit_tests()
{
    Freelist fl;
    freelist_init(&fl, malloc(1024*1024), 1024*1024, DEFAULT_ALIGNMENT);

    stack_int s = sinit(&fl.api, int);

    assert(spop(&s, int) == 0);

    spush(&s, int, 1);
    assert(spop(&s, int) == 1);

    for (int i = 1; i <= s.capacity; i++) {
        spush(&s, int, i);
    }
    for (int i = s.capacity; i > 0; i--) {
        assert(spop(&s, int) == i);
    }
    assert(stack_int_empty(&s));

    srand((unsigned)time(NULL));
    int counter = 0;
    for (int i = 0; i < 100000; i++) {
        if (rand() % 2) {
            spush(&s, int, counter++);
        } else if (s.top >= 0) {
            spop(&s, int);
        }
    }

    free(fl.data);
}

#endif
#endif
#endif