#ifndef BITMAPPED_ALLOC_H
#define BITMAPPED_ALLOC_H

#include <cstdlib>
#include <stdio.h>

// 128 4-byte blocks
#define SIZE_CLASS_BYTES 4
#define BUFFER_LEN 128
// each mask is 4 bytes long irrespective of the size_class
#define MASK_LEN BUFFER_LEN / 32

typedef struct bitmap_allocator bitmap_allocator;

struct bitmap_allocator {
    void *buffer;
    // need 128 bits for this memory
    // lsb of the individual mask tell the first index that is free.
    unsigned int mask[MASK_LEN];
};

void bitmap_allocator_init(bitmap_allocator *bma) {
    bma->buffer = malloc(BUFFER_LEN * SIZE_CLASS_BYTES);

    // setting all 1s since the entire buffer is free.
    for (int i = 0; i < MASK_LEN; ++i) {
        bma->mask[i] = -1;
    }
}

void *bitmap_allocator_alloc(bitmap_allocator *bma) {
    unsigned int mask_index = -1;
    for (int i = 0; i < MASK_LEN; ++i) {
        if (bma->mask[i]) {
            mask_index = i;
            break;
        }
    }
    
    // index of the first set bit from the right.
    unsigned int index = -1;
    if (bma->mask[mask_index]) {
        index = __builtin_ctz(bma->mask[mask_index]);
    }

    void *mem = (void *)((unsigned long long)bma->buffer + ((index + mask_index*32) * SIZE_CLASS_BYTES));
    bma->mask[mask_index] &= ~(1 << index);

    return mem;
} 

void bitmap_allocator_free(bitmap_allocator *bma, void *ptr) {
    unsigned int buffer_index = ((unsigned long long)ptr - (unsigned long long)bma->buffer) / SIZE_CLASS_BYTES;
    unsigned int mask_index = buffer_index / 32;
    unsigned int bit_index = buffer_index % 32;
    bma->mask[mask_index] |= (1 << bit_index);
}

void driver22() {

    bitmap_allocator alloc;
    bitmap_allocator_init(&alloc);

    void *mem1 = bitmap_allocator_alloc(&alloc);
    *((unsigned int *)mem1) = 200;
    void *mem2 = bitmap_allocator_alloc(&alloc);
    *((unsigned int *)mem2) = 201;
    void *mem3 = bitmap_allocator_alloc(&alloc);
    *((unsigned int *)mem3) = 202;
    void *mem4 = bitmap_allocator_alloc(&alloc);
    *((unsigned int *)mem4) = 203;
    
    bitmap_allocator_free(&alloc, mem3);
    void *mem5 = bitmap_allocator_alloc(&alloc);

    // should be 202
    unsigned int value = *((unsigned int *)mem5);
    int x = 0;
}

#endif