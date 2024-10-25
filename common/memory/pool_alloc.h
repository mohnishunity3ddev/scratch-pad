#ifndef POOL_ALLOC_H
#define POOL_ALLOC_H

#include "common.h"

typedef struct Pool_Free_Node Pool_Free_Node;
struct Pool_Free_Node
{
    Pool_Free_Node *next;
};

typedef struct Pool Pool;
struct Pool {
    unsigned char *buf;
    size_t buf_len;
    size_t chunk_size;

    Pool_Free_Node *head;
};

void  pool_init(Pool *p, void *backing_buffer, size_t backing_buffer_length, size_t chunk_size,
                size_t chunk_alignment);
void *pool_alloc(Pool *p);
void  pool_free(Pool *p, void *ptr);
void  pool_free_all(Pool *p);
void  pool_alloc_test();

#ifdef POOL_ALLOCATOR_IMPLEMENTATION
void
pool_init(Pool *p, void *backing_buffer, size_t backing_buffer_length, size_t chunk_size, size_t chunk_alignment)
{
    // align backing buffer to the specified chunk alignment
    uintptr_t initial_start = (uintptr_t)backing_buffer;
    uintptr_t start = align_forward(initial_start, (uintptr_t)chunk_alignment);
    backing_buffer_length -= (size_t)(start - initial_start);

    // align chunk size up to the required chunk_alignment
    chunk_size = align_forward(chunk_size, chunk_alignment);

    // assert parameters passed are valid.
    assert(chunk_size >= sizeof(Pool_Free_Node) && "Chunk size is too small");
    assert(backing_buffer_length >= chunk_size && "Backing Buffer Size is smaller than the chunk size");

    // Store the adjusted parameters
    p->buf = (unsigned char *)backing_buffer;
    p->buf_len = backing_buffer_length;
    p->chunk_size = chunk_size;
    p->head = NULL;

    // set up free list for free chunks.
    pool_free_all(p);
}

void *
pool_alloc(Pool *p)
{
    // Get the latest free node.
    Pool_Free_Node *node = p->head;

    if (node == NULL) {
        assert(0 && "Pool Allocator has no free memory");
        return NULL;
    }

    // pop free node
    p->head = p->head->next;

    // zero memory by default
    memset((void *)node, 0, p->chunk_size);
    return (void *)node;
}

void
pool_free(Pool *p, void *ptr)
{
    Pool_Free_Node *node;

    void *start = p->buf;
    void *end = &p->buf[p->buf_len];

    if (ptr == NULL) {
        // Ignore NULL Pointers.
        return;
    }

    if (!(start <= ptr && ptr < end)) {
        assert(0 && "Memory is out of bounds of the buffer in this pool");
        return;
    }

    // push free node to the free list.
    node = (Pool_Free_Node *)ptr;
    node->next = p->head;
    p->head = node;
}

void
pool_free_all(Pool *p)
{
    size_t chunk_count = p->buf_len / p->chunk_size;
    size_t i;

    for (int i = 0; i < chunk_count; ++i) {
        void *ptr = &p->buf[i * p->chunk_size];
        Pool_Free_Node *node = (Pool_Free_Node *)ptr;

        // push free node onto the free list.
        node->next = p->head;
        p->head = node;
    }
}

#ifdef POOL_ALLOCATOR_UNIT_TEST
void
pool_alloc_test()
{
    Pool pool;
    char *s = "hello \"you!\"";
    size_t backing_buffer_size = 48;
    void *backing_buffer = calloc(1, backing_buffer_size);
    // Make a pool of 32 integer arrays
    pool_init(&pool, backing_buffer, backing_buffer_size, sizeof(int) * 3, DEFAULT_ALIGNMENT);

#define ALLOC_TYPE(type) (type *)pool_alloc(&pool)
    int *arr1 = ALLOC_TYPE(int);
    for (int i = 0; i < 3; ++i) arr1[i] = i+1;

    int *arr2 = ALLOC_TYPE(int);
    for (int i = 0; i < 3; ++i) arr2[i] = i+2;

    pool_free(&pool, arr1);
    int *arr3 = ALLOC_TYPE(int);
    for (int i = 0; i < 3; ++i)
        arr3[i] = i + 3;

    int *arr4 = ALLOC_TYPE(int);
    for (int i = 0; i < 3; ++i) arr4[i] = i+4;

    pool_free_all(&pool);
    free(backing_buffer);
    int x = 0;
#undef ALLOC_TYPE
}
#endif

#endif
#endif