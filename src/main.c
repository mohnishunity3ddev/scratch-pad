// #define STB_DS_IMPLEMENTATION
// #include <stb/stb_ds.h>
// #include <stdio.h>

// #define LINEAR_ALLOCATOR_IMPLEMENTATION
// #define LINEAR_ALLOCATOR_UNIT_TESTS
// #include <memory/linear_alloc.h>
// #define STACK_ALLOCATOR_IMPLEMENTATION
// #define STACK_ALLOCATOR_UNIT_TEST
// #include <memory/stack_alloc_forward.h>
// #define POOL_ALLOCATOR_IMPLEMENTATION
// #define POOL_ALLOCATOR_UNIT_TEST
// #include <memory/pool_alloc.h>
#include "common.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <memory/memory.h>

#ifndef HASTHABLE_IMPLEMENTATION
#define HASHTABLE_IMPLEMENTATION
#endif
#include <containers/htable.h>

#define FREELIST_ALLOCATOR_UNIT_TESTS
#define FREELIST_ALLOCATOR_IMPLEMENTATION
#include <memory/freelist_alloc.h>
#define FREELIST2_ALLOCATOR_UNIT_TESTS
#define FREELIST2_ALLOCATOR_IMPLEMENTATION
#include <memory/freelist2_alloc.h>

#define STRING32_IMPLEMENTATION
#include <containers/string_utils.h>


#define RBT_IMPLEMENTATION
#include <containers/rb_tree.h>


#include <windows.h>
int main() {
    LARGE_INTEGER frequency, start, end;
    double elapsed_time;
    QueryPerformanceFrequency(&frequency);

    QueryPerformanceCounter(&start);
    freelist_unit_tests();
    freelist2_unit_tests();
    QueryPerformanceCounter(&end);
    elapsed_time += (double)(end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart;

    printf("Elapsed time: %.3f milliseconds\n", elapsed_time/1000.0);
    return 0;
}