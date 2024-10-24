// #define STB_DS_IMPLEMENTATION
// #include <stb/stb_ds.h>
// #include <stdio.h>

#define LINEAR_ALLOCATOR_IMPLEMENTATION
#define LINEAR_ALLOCATOR_UNIT_TESTS
#include <memory/linear_alloc.h>

int main()
{
    arena_unit_test();
}
