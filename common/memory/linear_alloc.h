#ifndef LINEAR_ALLOC_H
#define LINEAR_ALLOC_H
#include "common.h"

typedef struct Arena Arena;
struct Arena {
    unsigned char  *buf;         // start of the allocator block memory.
    size_t          buf_len;     // length of the buffer
    size_t          prev_offset; // offset into the buffer where last allocation started.
    size_t          curr_offset; // offset into the buffer where new allocation might start(if aligned).
};

typedef struct Temp_Arena_Memory Temp_Arena_Memory;
struct Temp_Arena_Memory {
    Arena *arena;
    size_t prev_offset;
    size_t curr_offset;
};


// #define LINEAR_ALLOCATOR_UNIT_TESTS

/// @brief initialize the arena with a pre-allocated memory buffer
/// @param a
/// @param backing_buffer
/// @param backing_buffer_length
void arena_init(Arena *a, void *backing_buffer, size_t backing_buffer_length);

/// @brief simply increments an offset to indicate the current buffer offset
/// @param a
/// @param size
/// @param align
/// @return
void *arena_alloc_align(Arena *a, size_t size, size_t align);
void *arena_alloc(Arena *a, size_t size);

/// @brief does absolutely nothing (just there for completeness)
/// @param a
/// @param ptr
void arena_free(Arena *a, void *ptr);

/// @brief first checks to see if the allocation being resized was the previously performed allocation and if so,
/// the same pointer will be returned and the buffer offset is changed. Otherwise, arena_alloc will be called
/// instead.
/// @param a
/// @param old_memory
/// @param old_size
/// @param new_size
/// @param align
/// @return
void *arena_resize_align(Arena *a, void *old_memory, size_t old_size, size_t new_size, size_t align);
void *arena_resize(Arena *a, void *old_memory, size_t old_size, size_t new_size);

/// @brief is used to free all the memory within the allocator by setting the buffer offsets to zero.
/// @param a
void arena_free_all(Arena *a);

void arena_unit_test();


Temp_Arena_Memory temp_arena_memory_begin(Arena *a);
void temp_arena_memory_end(Temp_Arena_Memory temp);

#if defined(LINEAR_ALLOCATOR_IMPLEMENTATION)
void
arena_init(Arena *a, void *backing_buffer, size_t backing_buffer_length)
{
    a->buf = (unsigned char *)backing_buffer;
    a->buf_len = backing_buffer_length;
    a->curr_offset = 0;
    a->prev_offset = 0;
}

void *
arena_alloc_align(Arena *a, size_t size, size_t align)
{
    uintptr_t curr_ptr = (uintptr_t)a->buf + (uintptr_t)a->curr_offset;
    // align offset to the "align"-byte boundary
    uintptr_t offset = align_forward(curr_ptr, align);
    // offset is from the "start" of arena buffer assuming arena_buffer is aligned to the 'align'-byte boundary.
    offset -= (uintptr_t)a->buf;

    // is there enough space?
    if ((offset + size) <= a->buf_len)
    {
        void *ptr = &a->buf[offset];
        a->prev_offset = offset;
        a->curr_offset = offset + size;

        memset(ptr, 0, size);
        return ptr;
    }

    return NULL;
}

void *
arena_alloc(Arena *a, size_t size)
{
    return arena_alloc_align(a, size, DEFAULT_ALIGNMENT);
}

void
arena_free(Arena *a, void *ptr)
{
    // DO NOTHING
}

// resize the memory held by old_memory within the arena
void *
arena_resize_align(Arena *a, void *old_memory, size_t old_size, size_t new_size, size_t align)
{
    unsigned char *old_mem = (unsigned char *)old_memory;
    assert(is_power_of_2(align));

    if (old_mem == NULL || old_size == 0)
    {
        return arena_alloc_align(a, new_size, align);
    }
    else if (a->buf <= old_mem && old_mem < (a->buf + a->buf_len)) // old_mem is within the arena memory range
    {
        // if the oldMem passed in was the last allocated from the arena.
        if ((a->buf + a->prev_offset) == old_mem)
        {
            // zero out the new extra bytes.
            if (new_size > old_size)
            {
                memset(&a->buf[a->curr_offset], 0, (new_size-old_size));
            }
            a->curr_offset = a->prev_offset + new_size;
            return old_memory;
        }
        else
        {
            void *new_memory = arena_alloc_align(a, new_size, align);
            size_t copy_size = (old_size < new_size) ? old_size : new_size;
            memmove(new_memory, old_memory, copy_size);
            return new_memory;
        }
    }
    else
    {
        assert(0 && "Memory is out of bounds of the buffer in the arena");
        return NULL;
    }
}

void *
arena_resize(Arena *a, void *old_memory, size_t old_size, size_t new_size)
{
    return arena_resize_align(a, old_memory, old_size, new_size, DEFAULT_ALIGNMENT);
}

void
arena_free_all(Arena *a)
{
    a->curr_offset = 0;
    a->prev_offset = 0;
}

#if defined(LINEAR_ALLOCATOR_UNIT_TESTS)
void
arena_unit_test()
{
    Arena arena;
    size_t backing_buffer_size = 1024 * 1024;
    void *backing_buffer = malloc(backing_buffer_size);
    arena_init(&arena, backing_buffer, backing_buffer_size);

#define alloc_struct(type, count) (type *)arena_alloc(&arena, sizeof(type)*count)
#define realloc_struct(type, ptr, old_count, new_count) (type *)arena_resize(&arena, ptr, old_count*sizeof(type), new_count*sizeof(type))
    unsigned char *arr1 = alloc_struct(unsigned char, 4);
    for (unsigned char i = 0; i < 4; ++i)
        arr1[i] = i;

    unsigned char *arr2 = alloc_struct(unsigned char, 16);
    for (unsigned char i = 0; i < 16; ++i)
        arr2[i] = i;

    arr2 = realloc_struct(unsigned char, arr2, 16, 4);
    for (unsigned char i = 0; i < 4; ++i)
        arr2[i] = i*2;

    unsigned char *arr3 = alloc_struct(unsigned char, 6);
    for (unsigned char i = 0; i < 6; ++i)
        arr3[i] = i;

    arena_free_all(&arena);
    free(backing_buffer);
    int x = 0;
#undef alloc_struct
#undef realloc_struct
}
#endif

Temp_Arena_Memory
temp_arena_memory_begin(Arena *a)
{
    Temp_Arena_Memory temp;
    temp.arena = a;
    temp.prev_offset = a->prev_offset;
    temp.curr_offset = a->curr_offset;
    return temp;
}

void
temp_arena_memory_end(Temp_Arena_Memory temp)
{
    temp.arena->prev_offset = temp.prev_offset;
    temp.arena->curr_offset = temp.curr_offset;
}
#endif
#endif