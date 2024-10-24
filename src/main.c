// #define STB_DS_IMPLEMENTATION
// #include <stb/stb_ds.h>
// #include <stdio.h>

// #define LINEAR_ALLOCATOR_IMPLEMENTATION
// #define LINEAR_ALLOCATOR_UNIT_TESTS
// #include <memory/linear_alloc.h>
#define STACK_ALLOCATOR_IMPLEMENTATION
#define STACK_ALLOCATOR_UNIT_TEST
#include <memory/stack_alloc_forward.h>

int main()
{
    stack_alloc_test();
}
