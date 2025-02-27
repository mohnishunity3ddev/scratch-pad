#ifndef STACK_ALLOC_FORWARD_H
#define STACK_ALLOC_FORWARD_H

#include "common.h"

typedef struct Stack Stack;
struct Stack {
    unsigned char *buf;
    size_t buf_len;
    size_t offset;
};


void stack_init(Stack *s, void *backing_buffer, size_t backing_buffer_length);
void *stack_alloc_align(Stack *s, size_t size, size_t alignment);
void *stack_alloc(Stack *s, size_t size);
void stack_free(Stack *s, void *ptr);
void *stack_resize_align(Stack *s, void *ptr, size_t old_size, size_t new_size, size_t alignment);
void *stack_resize(Stack *s, void *ptr, size_t old_size, size_t new_size);
void stack_free_all(Stack *s);
void stack_alloc_test();

#ifdef STACK_ALLOCATOR_IMPLEMENTATION
typedef struct Stack_Allocation_Header Stack_Allocation_Header;
struct Stack_Allocation_Header
{
    // padding bytes before the header to keep the allocation aligned.
    uint8_t padding;
};

void
stack_init(Stack *s, void *backing_buffer, size_t backing_buffer_length)
{
    s->buf = (unsigned char *)backing_buffer;
    s->buf_len = backing_buffer_length;
    s->offset = 0;
}

void *
stack_alloc_align(Stack *s, size_t size, size_t alignment)
{
    uintptr_t curr_addr, next_addr;
    size_t padding;
    Stack_Allocation_Header *header;

    assert(is_power_of_2(alignment));

    if (alignment > 128)
        alignment = 128;

    curr_addr = (uintptr_t)s->buf + (uintptr_t)s->offset;
    padding = calc_padding_with_header(curr_addr, (uintptr_t)alignment, sizeof(Stack_Allocation_Header));
    if ((s->offset + padding + size) > s->buf_len) {
        // no space left.
        return NULL;
    }
    s->offset += padding;

    next_addr = curr_addr + (uintptr_t)padding;
    header = (Stack_Allocation_Header *)(next_addr - sizeof(Stack_Allocation_Header));
    header->padding = (uint8_t)padding;

    s->offset += size;

    assert((uintptr_t)next_addr % alignment == 0);
    memset((void *)next_addr, 0, size);
    return (void *)next_addr;
}

void *
stack_alloc(Stack *s, size_t size)
{
    return stack_alloc_align(s, size, DEFAULT_ALIGNMENT);
}

void
stack_free(Stack *s, void *ptr)
{
    if (ptr != NULL) {
        uintptr_t start, end, curr_addr;
        Stack_Allocation_Header *header;
        size_t prev_offset;

        start     = (uintptr_t)s->buf;
        end       = start + (uintptr_t)s->buf_len;
        curr_addr = (uintptr_t)ptr;

        if (!(start <= curr_addr && curr_addr < end)) {
            assert(0 && "Out of bounds memory address passed to the stack allocator (free)");
        }

        if (curr_addr >= start + (uintptr_t)s->offset) {
            return;
        }

        // free everything after the the current pointer.
        header = (Stack_Allocation_Header *)(curr_addr - sizeof(Stack_Allocation_Header));
        prev_offset = (size_t)(curr_addr - (uintptr_t)header->padding - start);
        s->offset = prev_offset;
    }
}

void *
stack_resize_align(Stack *s, void *ptr, size_t old_size, size_t new_size, size_t alignment)
{
    if (ptr == NULL) {
        return stack_alloc_align(s, new_size, alignment);
    } else if(new_size == 0) {
        stack_free(s, ptr);
        return NULL;
    } else {
        uintptr_t start, end, curr_addr;
        size_t min_size = old_size < new_size ? old_size : new_size;
        void *new_ptr = ptr;

        start     = (uintptr_t)s->buf;
        end       = start + (uintptr_t)s->buf_len;
        curr_addr = (uintptr_t)ptr;

        if (!(start <= curr_addr && curr_addr < end)) {
            assert(0 && "Out of bounds memory passed into the stack allocator!");
            return NULL;
        }

        // treat as double free.
        if (curr_addr >= start + (uintptr_t)s->offset) {
            return NULL;
        }

        if (old_size == new_size) {
            return ptr;
        }

        // checking to see if this allocation is on top of stack. assumes old_size was correctly given in here.
        uintptr_t last_alloc_addr = (uintptr_t)(s->buf + s->offset) - (uintptr_t)old_size;
        if (curr_addr == last_alloc_addr)
        {
            if (new_size > old_size) {
                size_t space_needed = new_size - old_size;
                if ((start + s->offset + space_needed) <= end) {
                    s->offset += space_needed;
                } else {
                    // no space
                    return NULL;
                }
            } else {
                size_t extra_space = old_size - new_size;
                s->offset -= extra_space;
            }
            return new_ptr;
        }

        new_ptr = stack_alloc_align(s, new_size, alignment);
        memmove(new_ptr, ptr, min_size);
        return new_ptr;
    }
}

void *
stack_resize(Stack *s, void *ptr, size_t old_size, size_t new_size)
{
    return stack_resize_align(s, ptr, old_size, new_size, DEFAULT_ALIGNMENT);
}

void
stack_free_all(Stack *s)
{
    s->offset = 0;
}

#ifdef STACK_ALLOCATOR_UNIT_TEST
void
stack_alloc_test()
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
        arr1[i] = i * 3 + 1;
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
#endif

#endif
#endif