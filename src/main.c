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

#define QUEUE_IMPLEMENTATION
#include <containers/queue.h>

#define STACK_IMPLEMENTATION
#include <containers/stack.h>

#define DARR_IMPLEMENTATION
#include <containers/darr.h>

#define RBT_IMPLEMENTATION
#define RBT_UNIT_TESTS
#include <containers/rb_tree.h>


int
main()
{
    red_black_tree_test();
}