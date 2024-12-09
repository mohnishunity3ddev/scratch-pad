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
#include <assert.h>

#define FREELIST_ALLOCATOR_IMPLEMENTATION
#include <memory/freelist_alloc.h>

#define STRING32_IMPLEMENTATION
#include <containers/string_utils.h>

#define HASHTABLE_IMPLEMENTATION
#define HASHTABLE_UNIT_TESTS
#include <containers/htable.h>

int
main()
{
    htable_unit_tests();
}