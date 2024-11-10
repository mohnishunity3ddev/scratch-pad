#ifndef DARR_H
#define DARR_H

#include "common.h"
#include "memory/memory.h"
#include <stdio.h>

#define DARR_API(T, name)                                                                                         \
    typedef struct darr_##name                                                                                    \
    {                                                                                                             \
        T *arr;                                                                                                   \
        size_t length;                                                                                            \
        size_t capacity;                                                                                          \
        const alloc_api *api;                                                                                     \
    } darr_##name;                                                                                                \
    darr_##name darr_##name##_create_cap(const alloc_api *api, size_t init_cap);                                  \
    darr_##name darr_##name##_create(const alloc_api *api);                                                       \
    T darr_##name##_get(const darr_##name *array, size_t index);                                                  \
    void darr_##name##_push(darr_##name *array, T element);                                                       \
    void darr_##name##_update(darr_##name *array, size_t index, T element);                                       \
    void darr_##name##_remove_at(darr_##name *array, size_t index);                                               \
    void darr_##name##_print(darr_##name *array);
#define DARR_API_DEFAULT(T) DARR_API(T,T)

#define arrinit(api,t) darr_##t##_create(api)
#define arrpush(a,t,v) darr_##t##_push(a,v)
#define arrget(a,t,i) darr_##t##_get(a,i)
#define arrput(a,t,i,v) darr_##t##_update(a,i,v)
#define arrdel(a,t,i) darr_##t##_remove_at(a,i)
#define arrprint(a,t) darr_##t##_print(a)

#define arrinit_p(api) darr_voidp_create(api)
#define arrpush_p(a,v) darr_voidp_push(a,v)
#define arrget_p(a,t,i) (t*)darr_voidp_get(a,i)
#define arrput_p(a,i,v) darr_voidp_update(a,i,v)
#define arrdel_p(a,i) darr_voidp_remove_at(a,i)

#define DARR_API_IMPLEMENTATION(T, name)                                                                          \
    static void darr_##name##_expand(darr_##name *array)                                                          \
    {                                                                                                             \
        assert(array != NULL);                                                                                    \
        array->capacity *= 2;                                                                                     \
        array->arr = shrealloc_arr(array->api, array->arr, T, array->capacity);                                   \
        assert(array->arr != NULL);                                                                               \
    }                                                                                                             \
                                                                                                                  \
    darr_##name darr_##name##_create_cap(const alloc_api *api, size_t init_cap)                                   \
    {                                                                                                             \
        darr_##name array;                                                                                        \
        array.api = api;                                                                                          \
        array.capacity = init_cap;                                                                                \
        array.length = 0;                                                                                         \
        array.arr = shalloc_arr(api, T, array.capacity);                                                          \
        assert(array.arr != NULL);                                                                                \
        return array;                                                                                             \
    }                                                                                                             \
                                                                                                                  \
    darr_##name darr_##name##_create(const alloc_api *api) { return darr_##name##_create_cap(api, 2); }           \
                                                                                                                  \
    T darr_##name##_get(const darr_##name *array, size_t index)                                                   \
    {                                                                                                             \
        assert(array != NULL);                                                                                    \
        if (index < 0 || index >= array->length)                                                                  \
        {                                                                                                         \
            printf("[darr_get]: index out of bounds.\n");                                                         \
            return (T){0};                                                                                        \
        }                                                                                                         \
        return array->arr[index];                                                                                 \
    }                                                                                                             \
                                                                                                                  \
    void darr_##name##_push(darr_##name *array, T element)                                                        \
    {                                                                                                             \
        assert(array != NULL);                                                                                    \
        if (array->length + 1 > array->capacity)                                                                  \
        {                                                                                                         \
            darr_##name##_expand(array);                                                                          \
        }                                                                                                         \
        array->arr[array->length++] = element;                                                                    \
    }                                                                                                             \
                                                                                                                  \
    void darr_##name##_update(darr_##name *array, size_t index, T element)                                        \
    {                                                                                                             \
        if (index < 0 || index >= array->length)                                                                  \
        {                                                                                                         \
            printf("[darr_get]: index out of bounds.\n");                                                         \
            return;                                                                                               \
        }                                                                                                         \
        array->arr[index] = element;                                                                              \
    }                                                                                                             \
                                                                                                                  \
    void darr_##name##_remove_at(darr_##name *array, size_t index)                                                \
    {                                                                                                             \
        if (index < 0 || index >= array->length)                                                                  \
        {                                                                                                         \
            printf("[darr_get]: index out of bounds.\n");                                                         \
            return;                                                                                               \
        }                                                                                                         \
        for (size_t i = index; i < array->length - 1; ++i)                                                        \
        {                                                                                                         \
            array->arr[i] = array->arr[i + 1];                                                                    \
        }                                                                                                         \
        --array->length;                                                                                          \
    }
#define DARR_API_IMPLEMENTATION_DEFAULT(T) DARR_API_IMPLEMENTATION(T, T)

DARR_API_DEFAULT(int)
DARR_API_DEFAULT(darr_int) // dynamic array of dynamic array of ints.
DARR_API(void*, voidp)
DARR_API_DEFAULT(darr_voidp)

#ifdef DARR_UNIT_TESTS
    void darr_unit_tests();
#endif


#ifdef DARR_IMPLEMENTATION
DARR_API_IMPLEMENTATION_DEFAULT(int)
DARR_API_IMPLEMENTATION_DEFAULT(darr_int)
DARR_API_IMPLEMENTATION(void*, voidp)
DARR_API_IMPLEMENTATION_DEFAULT(darr_voidp)

#ifdef DARR_UNIT_TESTS
#ifndef FREELIST_ALLOCATOR_IMPLEMENTATION
#define FREELIST_ALLOCATOR_IMPLEMENTATION
#endif
#include "memory/freelist_alloc.h"

    typedef struct test
    {
        int a;
        float b;
        char c;
        double d;
} test;
test create_test(int a, float b, char c, double d) {
    test t;
    t.a = a; t.b = b; t.c = c; t.d = d;
    return t;
}
DARR_API(test)
DARR_API_IMPLEMENTATION(test)
void
darr_test_print(darr_test *arr)
{
    printf("Printing Array[Length: %zu]: \n", arr->length);
    for (size_t i = 0; i < arr->length; ++i) {
        test t = darr_test_get(arr, i);
        printf("[%zu] := { a = %d, b = %f, c = %c, d = %g }, \n", i, t.a, t.b, t.c, t.d);
    }
}

void
darr_unit_tests()
{
    Freelist fl;
    freelist_init(&fl, malloc(1024*1024), 1024*1024, DEFAULT_ALIGNMENT);

    darr_test a = arrinit(&fl.api, test);
    arrpush(&a, test, create_test(1,1,'a',1));
    arrpush(&a, test, create_test(2,2,'b',2));
    arrpush(&a, test, create_test(3,3,'c',3));
    arrpush(&a, test, create_test(4,4,'d',4));
    arrpush(&a, test, create_test(5,5,'e',5));
    arrpush(&a, test, create_test(6,6,'f',6));
    arrpush(&a, test, create_test(7,7,'g',7));
    arrprint(&a, test);

    arrput(&a, test, 0, create_test(101, 101, 'z', 101));
    arrprint(&a, test);
    arrdel(&a, test, 5);
    arrprint(&a, test);
    arrdel(&a, test, 0);
    arrprint(&a, test);

    free(fl.data);
}

#endif // DARR_UNIT_TESTS
#endif // DARR_IMPLEMENTATION

#endif // DARR_H