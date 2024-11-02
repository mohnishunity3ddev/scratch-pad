#ifndef MEMORY_H
#define MEMORY_H

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2 * sizeof(void *))
#endif

typedef void *(*alloc_fn)(void *allocator, size_t size);
typedef void *(*alloc_align_fn)(void *allocator, size_t size, size_t alignment);
typedef void *(*realloc_align_fn)(void *allocator, void *ptr, size_t new_size, size_t alignment);
typedef void (*free_fn)(void *allocator, void *ptr);
typedef void (*free_all_fn)(void *allocator);

typedef struct alloc_api alloc_api;
struct alloc_api
{
    alloc_fn alloc;
    alloc_align_fn alloc_align;
    realloc_align_fn realloc_align;
    free_fn free;
    free_all_fn free_all;

    void *allocator;
    size_t alignment;
};

bool
is_power_of_2(uintptr_t x)
{
    bool result = ((x & (x - 1)) == 0);
    return result;
}

uintptr_t
align_forward(uintptr_t ptr, size_t align)
{
    uintptr_t p, a, modulo;
    assert(is_power_of_2(align));

    p = ptr;
    a = (uintptr_t)align;
    // p % a since a is a power of 2.
    modulo = (p & (a - 1));

    if (modulo != 0)
    {
        // p + (a-modulo) is divisible by p
        p += (a - modulo);
    }

    return p;
}

size_t
calc_padding_with_header(uintptr_t ptr, uintptr_t alignment, size_t header_size)
{
    uintptr_t p, a, modulo, padding;
    // assert(is_power_of_2(alignment));
    // Example: if header_size is 38, and padding for ptr to be aligned is 5(say)
    // then extra space needed is 38-5 = 33, alignment should be on 16 byte boundary(say)
    // so we need 33 bytes more to be aligned.
    // 33 % 16 = 1, next 16 byte boundary is 48
    // so 48 + 5 should be the padding.

    p = ptr;
    a = alignment;
    modulo = is_power_of_2(alignment) ? (p & (a - 1)) : (p % a); // p % a (a is a power of 2)

    padding = 0;
    if (modulo != 0)
        padding = a - modulo;

    uintptr_t needed_space = (uintptr_t)header_size;
    // if alignemnt padding accomodates the needed size.
    if (padding < needed_space)
    {
        needed_space -= padding;
        // needed_space % a != 0
        if ((needed_space & (a - 1)) != 0)
        {
            padding += a * (1 + (needed_space / a));
        }
        else
        {
            padding += a * (needed_space / a);
        }
    }

    return (size_t)padding;
}

#endif