#ifndef MEMORY_H
#define MEMORY_H

#ifdef HAS_SSE2
#include <emmintrin.h>
#endif
#ifdef HAS_AVX
#include <immintrin.h>
#endif

#include <cassert>
#include <climits>
#include <cstdbool>
#include <cstdint>
#include <cstdlib>
#include <cstring>


#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2 * sizeof(void *))
#endif

static char stringBuffer[512];
#ifdef _DEBUG
#include <stdio.h>
inline const char *
// getLabel(const char *s, const char *filename, int lineNumber)
getLabel(const char *s)
{
    memset(stringBuffer, 0, 512);
    // snprintf(stringBuffer, 512, "%s, File: %s, Line: %d", s/*, filename, lineNumber*/);
    snprintf(stringBuffer, 512, "%s", s);
    return stringBuffer;
}
#endif

#define KILOBYTES(v) ((v)*1024LL)
#define MEGABYTES(v) (KILOBYTES(v)*1024LL)
#define GIGABYTES(v) (MEGABYTES(v)*1024LL)
#define TERABYTES(v) (GIGABYTES(v)*1024LL)

typedef enum Placement_Policy
{
    PLACEMENT_POLICY_FIND_FIRST,
    PLACEMENT_POLICY_FIND_BEST,
} Placement_Policy;

#define shalloc_a(api,sz,a) (api != NULL ? api->alloc_align(api->allocator, sz, a) : malloc(sz))
#define shalloc(api,sz) shalloc_a(api, sz, DEFAULT_ALIGNMENT)
#define shrealloc_a(api,p,sz,a) (api != NULL ? api->realloc_align(api->allocator, p, sz, a) : realloc(p,sz))
#define shrealloc(api,p,sz) shrealloc_a(api,p,sz,api->alignment)
#define shfree(api,p) (api != NULL) ? api->free(api->allocator,p) : free(p)
#define shalloc_arr(api,t,c) (t *)shalloc(api,c*sizeof(t))
#define shrealloc_arr(api,p,t,c) (t *)shrealloc(api,p,c*sizeof(t))
#define shalloc_t(api,t) (t *)shalloc(api,sizeof(t))

typedef void *(*alloc_fn)(void *allocator, size_t size);
typedef void *(*alloc_align_fn)(void *allocator, size_t size, size_t alignment);
typedef void *(*realloc_align_fn)(void *allocator, void *ptr, size_t new_size, size_t alignment);
typedef void *(*realloc_fn)(void *allocator, void *ptr, size_t new_size);
typedef void  (*free_fn)(void *allocator, void *ptr);
typedef void  (*free_all_fn)(void *allocator);

typedef struct alloc_api {
    alloc_fn alloc;
    alloc_align_fn alloc_align;
    realloc_fn realloc;
    realloc_align_fn realloc_align;
    free_fn free;
    free_all_fn free_all;

    void *allocator;
    size_t alignment;
} alloc_api;

bool is_power_of_2(uintptr_t x);
uintptr_t align_forward(uintptr_t ptr, size_t align);
size_t calc_padding_with_header(uintptr_t ptr, uintptr_t alignment, size_t header_size);



inline bool
is_power_of_2(uintptr_t x)
{
    bool result = ((x & (x - 1)) == 0);
    return result;
}

inline uintptr_t
align_forward(uintptr_t ptr, size_t align)
{
    assert(is_power_of_2(align));
    uintptr_t modulo = (ptr & (align - 1));
    return !modulo ? ptr : ptr + (align-modulo);
}

inline size_t
calc_padding_with_header(uintptr_t ptr, uintptr_t alignment, size_t header_size)
{
    uintptr_t p, a, modulo, padding, needed_space;
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
    needed_space = (uintptr_t)header_size;

    // if alignemnt padding accomodates the needed size.
    if (padding < needed_space)
    {
        needed_space -= padding;
        // needed_space % a != 0
        if ((needed_space & (a - 1)) != 0) {
            padding += a * (1 + (needed_space / a));
        } else {
            padding += a * (needed_space / a);
        }
    }

    return (size_t)padding;
}


inline void
shumemcpy(void *destination, const void *src, size_t size)
{
    unsigned char *d = (unsigned char *)destination;
    const unsigned char *s = (const unsigned char *)src;

    // TODO: check if src and dst are aligned to apt boundary and use aligned SIMD instructions. Unaligned loadu and storeu instructions are slower.

#ifdef HAS_AVX512F
    while (size >= 64) {
        __m512i chunk = _mm512_loadu_si512((const __m512i *)s);
        _mm512_storeu_si512((__m512i *)d, chunk);
        s += 64; d += 64; size -= 64;
    }
#endif
#if defined(HAS_AVX)
    while (size >= 32) {
        __m256i chunk = _mm256_loadu_si256((const __m256i *)s);
        _mm256_storeu_si256((__m256i *)d, chunk);
        s += 32; d += 32; size -= 32;
    }
#endif
#if defined(HAS_SSE2)
    while (size >= 16) {
        __m128i chunk = _mm_load_si128((const __m128i *)s);
        _mm_storeu_si128((__m128i *)d, chunk);
        s += 16; d += 16; size -= 16;
    }
#endif
    while(size >= 8) {
        *((uint64_t*)d) = *((uint64_t*)s);
        s += 8; d += 8; size -= 8;
    }

    while (size > 0) {
        *d++ = *s++;
        size--;
    }
}

#endif
