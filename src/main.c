// #define STB_DS_IMPLEMENTATION
// #include <stb/stb_ds.h>
// #include <stdio.h>

// #define LINEAR_ALLOCATOR_IMPLEMENTATION
// #define LINEAR_ALLOCATOR_UNIT_TESTS
// #include <memory/linear_alloc.h>
#define STACK_ALLOCATOR_IMPLEMENTATION
#include <memory/stack_alloc_forward.h>

int main()
{
    Stack stack;
    size_t backing_buffer_size = 1024 * 1024;
    void *backing_buffer = malloc(backing_buffer_size);
    stack_init(&stack, backing_buffer, backing_buffer_size);

#define alloc_struct(type, count) (type *)stack_alloc(&stack, sizeof(type) * count)
#define realloc_struct(type, ptr, old_count, new_count)                                                           \
    (type *)stack_resize(&stack, ptr, old_count * sizeof(type), new_count * sizeof(type))
    unsigned char *arr1 = alloc_struct(unsigned char, 4);
    for (unsigned char i = 0; i < 4; ++i)
        arr1[i] = i*3+1;
    assert((size_t)arr1 % DEFAULT_ALIGNMENT == 0);

    unsigned char *arr2 = alloc_struct(unsigned char, 16);
    for (unsigned char i = 0; i < 16; ++i)
        arr2[i] = i;
    assert((size_t)arr2 % DEFAULT_ALIGNMENT == 0);

    // arr2 is the last allocation, checking to see if it gets resized in place.
    arr2 = realloc_struct(unsigned char, arr2, 16, 4);
    for (unsigned char i = 0; i < 4; ++i)
        arr2[i] = i * 2;
    assert((size_t)arr2 % DEFAULT_ALIGNMENT == 0);

    // arr1 is not on top of the stack, check to see if gets moved to a new location.
    arr1 = realloc_struct(unsigned char, arr1, 4, 16);
    for (unsigned char i = 0; i < 16; ++i)
        arr1[i] = i * 2;
    assert((size_t)arr1 % DEFAULT_ALIGNMENT == 0);

    unsigned char *arr3 = alloc_struct(unsigned char, 6);
    for (unsigned char i = 0; i < 6; ++i)
        arr3[i] = i;
    assert((size_t)arr3 % DEFAULT_ALIGNMENT == 0);

    stack_free_all(&stack);
    arr2 = alloc_struct(unsigned char, 16);
    for (unsigned char i = 0; i < 16; ++i)
        arr2[i] = i * 2;
    assert((size_t)arr2 % DEFAULT_ALIGNMENT == 0);

    stack_free_all(&stack);
    free(backing_buffer);
    int x = 0;
#undef alloc_struct
#undef realloc_struct
}
