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
#include <stdio.h>
#define FREELIST_ALLOCATOR_IMPLEMENTATION
#define FREELIST_ALLOCATOR_UNIT_TESTS
#include <memory/freelist_alloc.h>

int
main()
{
    freelist_tests();
}
