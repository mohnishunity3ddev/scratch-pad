#ifndef BUDDY_ALLOC_H
#define BUDDY_ALLOC_H

#include "memory.h"
#include "common.h"

typedef struct buddy_block {
    size_t size;
    bool is_free;
} buddy_block;

buddy_block *
buddy_block_next(buddy_block *block)
{
    buddy_block *result;
    result = (buddy_block *)((char *)block + block->size);
    return result;
}

buddy_block *
buddy_block_split(buddy_block *block, size_t size)
{
    if (block != NULL && size != 0) {
        // recursive split
        while (size < block->size) {
            size_t sz = block->size >> 1;
            block->size = sz;
            block = buddy_block_next(block);
            block->size = sz;
            block->is_free = true;
        }
        if (size <= block->size) {
            return block;
        }
    }
    // Block cannot fit the requested allocation size.
    return NULL;
}

buddy_block *
buddy_block_find_best(buddy_block *head, buddy_block *tail, size_t size)
{
    buddy_block *best_block = NULL;
    buddy_block *block = head;                    // Left Buddy
    buddy_block *buddy = buddy_block_next(block); // Right Buddy

    // The entire memory section between head and tail is free,
    // just call 'buddy_block_split' to get the allocation
    if (buddy == tail && block->is_free)
    {
        return buddy_block_split(block, size);
    }
    
    // Find the block which is the 'best_block' to requested allocation sized
    while (block < tail && buddy < tail) { // make sure the buddies are within the range
        // If both buddies are free, coalesce them together
        // NOTE: this is an optimization to reduce fragmentation
        //       this could be completely ignored
        if (block->is_free && buddy->is_free &&
            block->size == buddy->size)
        {
            block->size <<= 1;
            if (size <= block->size &&
                (best_block == NULL || block->size <= best_block->size)) {
                best_block = block;
            }
            // buddy here is the buddy of the current 'block', next block will come right after buddy finishes i.e.
            // buddy's buddy
            block = buddy_block_next(buddy);
            if (block < tail) {
                // Delay the buddy block for the next iteration
                buddy = buddy_block_next(block);
            }
            continue;
        }

        if (block->is_free && size <= block->size &&
            (best_block == NULL || block->size <= best_block->size)) {
            best_block = block;
        }

        if (buddy->is_free && size <= buddy->size && (best_block == NULL || buddy->size < best_block->size)) {
            // If each buddy are the same size, then it makes more sense
            // to pick the buddy as it "bounces around" less
            best_block = buddy;
        }

        if (block->size <= buddy->size) {
            block = buddy_block_next(buddy);
            if (block < tail) {
                // Delay the buddy block for the next iteration
                buddy = buddy_block_next(block);
            }
        } else {
            // Buddy was split into smaller blocks
            block = buddy;
            buddy = buddy_block_next(buddy);
        }
    }

    if (best_block != NULL) {
        // This will handle the case if the 'best_block' is also the perfect fit
        return buddy_block_split(best_block, size);
    }

    // Maybe out of memory
    return NULL;
}

typedef struct buddy_allocator
{
    buddy_block *head; // same pointer as the backing memory data
    buddy_block *tail; // sentinel pointer representing the memory boundary
    size_t alignment;
} buddy_allocator;

void
buddy_allocator_init(buddy_allocator *b, void *data, size_t size, size_t alignment)
{
    assert(data != NULL);
    assert(is_power_of_two(size) && "size is not a power-of-two");
    assert(is_power_of_two(alignment) && "alignment is not a power-of-two");

    // The minimum alignment depends on the size of the `buddy_block` header
    assert(is_power_of_two(sizeof(buddy_block)) == 0);
    if (alignment < sizeof(buddy_block))
    {
        alignment = sizeof(buddy_block);
    }
    assert((uintptr_t)data % alignment == 0 && "data is not aligned to minimum alignment");

    b->head = (buddy_block *)data;
    b->head->size = size;
    b->head->is_free = true;

    // The tail here is a sentinel value and not a true block
    b->tail = buddy_block_next(b->head);

    b->alignment = alignment;
}

size_t
buddy_block_size_required(buddy_allocator *b, size_t size)
{
    size_t actual_size = b->alignment;

    size += sizeof(buddy_block);
    size = align_forward_size(size, b->alignment);

    while (size > actual_size) {
        actual_size <<= 1;
    }

    return actual_size;
}

void
buddy_block_coalescence(buddy_block *head, buddy_block *tail)
{
    for (;;) {
        // Keep looping until there are no more buddies to coalesce

        buddy_block *block = head;
        buddy_block *buddy = buddy_block_next(block);

        bool no_coalescence = true;
        while (block < tail && buddy < tail)
        { // make sure the buddies are within the range
            if (block->is_free && buddy->is_free && block->size == buddy->size) {
                // Coalesce buddies into one
                block->size <<= 1;
                block = buddy_block_next(block);
                if (block < tail) {
                    buddy = buddy_block_next(block);
                    no_coalescence = false;
                }
            } else if (block->size < buddy->size) {
                // The buddy block is split into smaller blocks
                block = buddy;
                buddy = buddy_block_next(buddy);
            } else {
                block = buddy_block_next(buddy);
                if (block < tail) {
                    // Leave the buddy block for the next iteration
                    buddy = buddy_block_next(block);
                }
            }
        }

        if (no_coalescence) {
            return;
        }
    }
}

void *
buddy_allocator_alloc(buddy_allocator *b, size_t size)
{
    if (size != 0) {
        size_t actual_size = buddy_block_size_required(b, size);

        buddy_block *found = buddy_block_find_best(b->head, b->tail, actual_size);
        if (found == NULL) {
            // Try to coalesce all the free buddy blocks and then search again
            buddy_block_coalescence(b->head, b->tail);
            found = buddy_block_find_best(b->head, b->tail, actual_size);
        }

        if (found != NULL) {
            found->is_free = false;
            return (void *)((char *)found + b->alignment);
        }

        // Out of memory (possibly due to too much internal fragmentation)
    }

    return NULL;
}

void
buddy_allocator_free(buddy_allocator *b, void *data)
{
    if (data != NULL) {
        buddy_block *block;

        assert(b->head <= data);
        assert(data < b->tail);

        block = (buddy_block *)((char *)data - b->alignment);
        block->is_free = true;

        // NOTE: Coalescence could be done now but it is optional
        buddy_block_coalescence(b->head, b->tail);
    }
}

void
buddy_unit_tests()
{
    // Initialize with 64MB of memory
    const size_t mem_size = 64 * 1024 * 1024;
    void *memory = malloc(mem_size);
    assert(memory != NULL);

    buddy_allocator buddy;
    buddy_allocator_init(&buddy, memory, mem_size, DEFAULT_ALIGNMENT);
}

#endif