#ifndef FREELIST_ALLOC_H
#define FREELIST_ALLOC_H

#include "common.h"

typedef struct Freelist_Allocation_Header Freelist_Allocation_Header;
struct Freelist_Allocation_Header {
    /// @brief this includes required size, size for header and size for alignment padding.
    size_t block_size;
    /// @brief size for alignment only
    size_t alignment_padding;
};

typedef struct Freelist_Node Freelist_Node;
struct Freelist_Node {
    Freelist_Node *next;
    size_t block_size;
};

typedef enum Placement_Policy {
    PLACEMENT_POLICY_FIND_FIRST,
    PLACEMENT_POLICY_FIND_BEST,
} Placement_Policy;

typedef struct Freelist Freelist;

void  freelist_init(Freelist *fl, void *data, size_t size, size_t alignment);
void *freelist_alloc(void *fl, size_t size);
void *freelist_alloc_align(void *fl, size_t size, size_t alignment);
void  freelist_free(void *fl, void *ptr);
void *freelist_realloc(void *fl, void *ptr, size_t new_size, size_t alignment);
void  freelist_free_all(void *fl);
size_t freelist_remaining_space(Freelist *fl);

struct Freelist {
    alloc_api api;

    void *data;
    size_t size;
    size_t used;
    Freelist_Node *head;

    Placement_Policy policy;
    int block_count;
};

#ifdef FREELIST_ALLOCATOR_UNIT_TESTS
void freelist_unit_tests();
#endif

#ifdef FREELIST_ALLOCATOR_IMPLEMENTATION
void  freelist_node_insert(Freelist *fl, Freelist_Node *prev_node, Freelist_Node *new_node);
void  freelist_node_remove(Freelist *fl, Freelist_Node *prev_node, Freelist_Node *del_node);
void *freelist_realloc_sized(void *fl, void *ptr, size_t old_size, size_t new_size, size_t alignment);

size_t freelist_block_size(void *ptr) {
    assert(ptr != NULL);
    Freelist_Allocation_Header *header = (Freelist_Allocation_Header *)((char *)ptr -
                                                                        sizeof(Freelist_Allocation_Header));
    size_t result = header->block_size;
    return result;
}

size_t
freelist_remaining_space(Freelist *fl)
{
    Freelist_Node *curr = fl->head;
    size_t space = 0;
    while (curr != NULL) {
        space += curr->block_size;
        curr = curr->next;
    }
    return space;
}

void
freelist_free_all(void *fl)
{
    Freelist *freelist = (Freelist *)fl;
    freelist->used = 0;
    Freelist_Node *first_node = (Freelist_Node *)freelist->data;
    first_node->block_size = freelist->size;
    first_node->next = NULL;
    freelist->head = first_node;
    freelist->block_count = 1;
}

void
freelist_init(Freelist *fl, void *data, size_t size, size_t alignment)
{
    fl->data = data;
    fl->size = size;
    freelist_free_all(fl);

    fl->api.alloc = freelist_alloc;
    fl->api.alloc_align = freelist_alloc_align;
    fl->api.realloc_align = freelist_realloc;
    fl->api.free = freelist_free;
    fl->api.free_all = freelist_free_all;

    fl->api.alignment = alignment;
    fl->api.allocator = (void *)fl;
}

Freelist_Node *
freelist_find_first(Freelist *fl, size_t size, size_t alignment, size_t *padding_, Freelist_Node **prev_node_)
{
    Freelist_Node *node = fl->head;
    Freelist_Node *prev_node = NULL;

    size_t padding = 0;

    while(node != NULL) {
        padding = calc_padding_with_header((uintptr_t)node, alignment, sizeof(Freelist_Allocation_Header));
        size_t required_space = size + padding;

        if (node->block_size >= required_space) {
            break;
        }
        prev_node = node;
        node = node->next;
    }

    if (padding_)   *padding_ = padding;
    if (prev_node_) *prev_node_ = prev_node;
    return node;
}

Freelist_Node *
freelist_find_best(Freelist *fl, size_t size, size_t alignment, size_t *padding_, Freelist_Node **prev_node_)
{
    size_t smallest_diff = ~(size_t)0;

    Freelist_Node *node = fl->head,
                  *prev_node = NULL,
                  *best_node = NULL;

    size_t padding = 0;
    while (node != NULL) {
        padding = calc_padding_with_header((uintptr_t)node, (uintptr_t)alignment,
                                           sizeof(Freelist_Allocation_Header));
        size_t required_space = size + padding;
        if ((node->block_size >= required_space) &&
            (node->block_size - required_space) < smallest_diff)
        {
            best_node = node;
            smallest_diff = (node->block_size - required_space);
            break;
        }
        prev_node = node;
        node = node->next;
    }

    if (padding_) *padding_ = padding;
    if (prev_node_) *prev_node_ = prev_node;
    return best_node;
}

void *
freelist_alloc(void *fl, size_t size)
{
    return freelist_alloc_align(fl, size, DEFAULT_ALIGNMENT);
}

void *
freelist_alloc_align(void *fl, size_t size, size_t alignment)
{
    assert(fl != NULL);
    Freelist *freelist = (Freelist *)fl;

    size_t padding = 0;
    Freelist_Node *prev_node = NULL;
    Freelist_Node *node = NULL;

    size_t alignment_padding, required_space, remaining;
    Freelist_Allocation_Header *header_ptr;
    size_t alloc_header_size = sizeof(Freelist_Allocation_Header);

    // we have to save info about the free node which this alloc will be when we free it. So we need enough space
    // to store free_node info.
    if (size < sizeof(Freelist_Node)) size = sizeof(Freelist_Node);
    if (alignment < 8) alignment = 8;

    if (freelist->policy == PLACEMENT_POLICY_FIND_BEST) {
        node = freelist_find_best(freelist, size, alignment, &padding, &prev_node);
    } else if (freelist->policy == PLACEMENT_POLICY_FIND_FIRST) {
        node = freelist_find_first(freelist, size, alignment, &padding, &prev_node);
    } else {
        assert(0 && "No Valid placement policy set!");
    }

    if (node == NULL) {
        printf("Freelist has no memory.\n");
        return NULL;
    }

    assert(padding >= alloc_header_size);
    alignment_padding = padding - alloc_header_size;

    required_space = size + padding;
    assert(node->block_size >= required_space);
    remaining = node->block_size - required_space;

    if (remaining > 0) {
        // a free node should have more than memory than is required to store header(metadata information).
        if (remaining > sizeof(Freelist_Allocation_Header) &&
            remaining > sizeof(Freelist_Node))
        {
            Freelist_Node *new_node = (Freelist_Node *)((char *)node + required_space);
            new_node->block_size = remaining;
            freelist_node_insert(freelist, node, new_node);
        } else {
            // since we can't use the remaining space effectively, we just add to the allocated block--the
            // remaining space.
            required_space += remaining;
        }
    }

    freelist_node_remove(freelist, prev_node, node);

    header_ptr = (Freelist_Allocation_Header *)((char *)node + alignment_padding);
    header_ptr->block_size = required_space;
    header_ptr->alignment_padding = alignment_padding;

    freelist->used += required_space;

    void *memory = (void *)((char *)header_ptr + alloc_header_size);
    return memory;
}

void freelist_coalescence(Freelist *fl, Freelist_Node *prev_node, Freelist_Node *free_node);

void
freelist_free(void *fl, void *ptr)
{
    assert(fl != NULL && ptr != NULL);
    Freelist *freelist = (Freelist *)fl;

    Freelist_Allocation_Header *header;
    Freelist_Node *free_node = NULL;

    if (ptr == NULL) {
        return;
    }

    header = (Freelist_Allocation_Header *)((char *)ptr - sizeof(Freelist_Allocation_Header));

    // the actual header comes after padding.
    free_node = (Freelist_Node *)((char *)header - header->alignment_padding);
    free_node->block_size = header->block_size;
    free_node->next = NULL;

    assert(freelist->used >= free_node->block_size);
    freelist->used -= free_node->block_size;
    if (freelist->head != NULL) {
        Freelist_Node *node = freelist->head;
        Freelist_Node *prev_node = NULL;
        while (node != NULL) {
            if (ptr < (void *)node) {
                freelist_node_insert(freelist, prev_node, free_node);
                break;
            }
            prev_node = node;
            node = node->next;
        }
        freelist_coalescence(freelist, prev_node, free_node);
    } else {
        freelist->head = free_node;
        freelist->block_count = 1;
    }
}

void
freelist_coalescence(Freelist *fl, Freelist_Node *prev_node, Freelist_Node *free_node)
{
    if ((free_node->next != NULL) &&
        (void *)((char *)free_node + free_node->block_size) == free_node->next)
    {
        free_node->block_size += free_node->next->block_size;
        assert(fl->block_count >= 2);
        freelist_node_remove(fl, free_node, free_node->next);
    }

    if ((prev_node != NULL && prev_node->next != NULL) &&
        (void *)((char *)prev_node + prev_node->block_size) == free_node)
    {
        prev_node->block_size += free_node->block_size;
        assert(fl->block_count >= 2);
        freelist_node_remove(fl, prev_node, free_node);
    }
    assert(fl->block_count > 0);
}

void *
freelist_realloc(void *fl, void *ptr, size_t new_size, size_t alignment)
{
    assert(fl != NULL);
    if (ptr == NULL) {
        // should behave like alloc.
        return freelist_alloc_align(fl, new_size, alignment);
    }
    if (new_size == 0) {
        freelist_free(fl, ptr);
        return NULL;
    }

    if ((uintptr_t)ptr % alignment != 0) {
        printf("Misaligned memory!\n");
        return NULL;
    }
    Freelist *freelist = (Freelist *)fl;
    if ((uintptr_t)ptr < (uintptr_t)(freelist->data) ||
        (uintptr_t)ptr > ((uintptr_t)freelist->data + (uintptr_t)freelist->size))
    {
        printf("Out of Bounds Memory Access!\n");
        return NULL;
    }
    size_t alloc_header_size = sizeof(Freelist_Allocation_Header);
    Freelist_Allocation_Header *header = (Freelist_Allocation_Header *)((char *)ptr - alloc_header_size);
    size_t header_align_padding = alloc_header_size + header->alignment_padding;
    size_t old_size             = header->block_size - header_align_padding;
    if (old_size == new_size) {
        return ptr;
    }
    size_t remaining_space = freelist_remaining_space(freelist);
    assert(remaining_space >= sizeof(Freelist_Allocation_Header));
    if ((new_size > old_size) &&
        (new_size - old_size) > (remaining_space - sizeof(Freelist_Allocation_Header)))
    {
        printf("Not enough space in the backing buffer!\n");
        return NULL;
    }

    void *new_ptr = freelist_realloc_sized(fl, ptr, old_size, new_size, alignment);

    assert((freelist_remaining_space(freelist) + freelist->used) == freelist->size);
    return new_ptr;
}

void *
freelist_realloc_sized(void *fl, void *ptr, size_t old_size, size_t new_size, size_t alignment)
{
    assert(fl != NULL && ptr != NULL);

    Freelist *freelist = (Freelist *)fl;
    Freelist_Allocation_Header *alloc_header = (Freelist_Allocation_Header *)((char *)ptr -
                                                                              sizeof(Freelist_Allocation_Header));
    if (old_size < new_size)
    {
        size_t extra_space = new_size - old_size;
        for (Freelist_Node *curr = freelist->head, *prev = NULL;
             curr != NULL;
             prev = curr, curr = curr->next)
        {
            if ((uintptr_t)curr == ((uintptr_t)ptr + old_size) &&
                (curr->block_size >= extra_space))
            {
                const size_t remaining = curr->block_size - extra_space;
                if (remaining >= sizeof(Freelist_Allocation_Header))
                {
                    Freelist_Node *new_node = (Freelist_Node *)((char *)curr + extra_space);
                    new_node->next = curr->next;
                    new_node->block_size = remaining;
                    if (prev) {
                        prev->next = new_node;
                    } else {
                        freelist->head = new_node;
                    }
                } else {
                    // remaining size in the free block is too small to even store freeblock metadata. Add it to
                    // the current allocation and remove the whole free block from the freelist.
                    extra_space += remaining;
                    freelist_node_remove(freelist, prev, curr);
                }
                alloc_header->block_size += extra_space;
                freelist->used += extra_space;
                return ptr;
            }
        }

        void *new_ptr = freelist_alloc_align(freelist, new_size, alignment);
        if (new_ptr) {
            memcpy(new_ptr, ptr, old_size);
            freelist_free(freelist, ptr);
            return new_ptr;
        }
        return NULL;
    }

    assert(old_size > new_size);
    const size_t free_space = old_size - new_size;

    if (free_space < sizeof(Freelist_Node)) {
        // cannot make a free node just containing free_space.
        // checking if there is a free block immediately after old allocation.
        Freelist_Node *curr = freelist->head;
        Freelist_Node *prev = NULL;
        while (curr != NULL && (uintptr_t)curr < (uintptr_t)ptr) {
            prev = curr;
            curr = curr->next;
        }
        char *addr_after_allocation = (char *)ptr + old_size;
        if ((uintptr_t)curr == (uintptr_t)(addr_after_allocation)) {
            // lining up
            size_t new_block_size = curr->block_size + free_space;
            Freelist_Node *new_node = (Freelist_Node *)(addr_after_allocation - free_space);
            new_node->next = curr->next;
            new_node->block_size = new_block_size;
            if (prev) {
                prev->next = new_node;
            } else {
                freelist->head = new_node;
            }
            assert(freelist->used > free_space);
            freelist->used -= free_space;
            assert(alloc_header->block_size > free_space);
            alloc_header->block_size -= free_space;
        } else {
            // DO NOTHING. can't save an isolated block which has less size than what is required to store free
            // block metadata.
        }
        return ptr;
    }

    assert(alloc_header->block_size > free_space);
    alloc_header->block_size -= free_space;

    Freelist_Node *new_node = (Freelist_Node *)((char *)ptr + new_size);
    new_node->block_size = free_space;
    new_node->next = NULL;

    assert(freelist->used >= free_space);
    freelist->used -= free_space;

    Freelist_Node *curr = freelist->head;
    Freelist_Node *prev = NULL;

    while (curr && (uintptr_t)curr < (uintptr_t)new_node) {
        prev = curr;
        curr = curr->next;
    }

    freelist_node_insert(freelist, prev, new_node);

    if (new_node->next &&
        (char *)new_node + new_node->block_size == (char *)new_node->next)
    {
        new_node->block_size += new_node->next->block_size;
        freelist_node_remove(freelist, new_node, new_node->next);
    }

    if (prev &&
        (char *)prev + prev->block_size == (char *)new_node)
    {
        prev->block_size += new_node->block_size;
        freelist_node_remove(freelist, prev, new_node);
    }

    return ptr;
}

void
freelist_node_insert(Freelist *fl, Freelist_Node *prev_node, Freelist_Node *new_node)
{
    assert(prev_node != new_node);

    if (prev_node == NULL) {
        if (fl->head != NULL) {
            new_node->next = fl->head;
        }
        fl->head = new_node;
    } else {
        if (prev_node->next == NULL) {
            prev_node->next = new_node;
            new_node->next = NULL;
        } else {
            new_node->next = prev_node->next;
            prev_node->next = new_node;
        }
    }
    ++fl->block_count;
    assert(fl->block_count > 0);
}

void
freelist_node_remove(Freelist *fl, Freelist_Node *prev_node, Freelist_Node *del_node)
{
    if (prev_node == NULL) {
        fl->head = del_node->next;
    } else {
        prev_node->next = del_node->next;
    }
    --fl->block_count;
    assert(fl->block_count >= 0);
}

#ifdef FREELIST_ALLOCATOR_UNIT_TESTS
static void
freelist_test_initial_state(Freelist *fl)
{
    assert(fl->head == fl->data);
    assert(fl->block_count == 1);
    assert(fl->used == 0);
    assert(fl->head->block_size == fl->size);
}

static void
verify_freelist_blocks(Freelist *fl, size_t *exp_free_sizes, int exp_block_count, bool print_block_size)
{
    int counter = 0;
    Freelist_Node *curr = fl->head;
    while (curr && curr->next)
    {
        if (print_block_size) {
            printf("%zu-->", curr->block_size);
        }
        assert(exp_free_sizes[counter] == curr->block_size);
        curr = curr->next;
        ++counter;
    }
    assert(counter + 1 == exp_block_count);
    if (print_block_size) {
        printf(".\n");
    }
}

static void
validate_freelist_order(Freelist *fl)
{
    Freelist_Node *current = fl->head;
    while (current && current->next)
    {
        // Verify addresses are in ascending order
        assert((uintptr_t)current < (uintptr_t)current->next);
        // Verify no overlapping blocks
        assert((uintptr_t)current + current->block_size <= (uintptr_t)current->next);
        current = current->next;
    }
}

// Helper function to validate used memory accounting
static void
validate_used_memory(Freelist *fl, void *base_addr, size_t total_size)
{
    size_t free_mem = 0;
    Freelist_Node *current = fl->head;

    // Sum up all free blocks
    while (current)
    {
        free_mem += current->block_size;
        current = current->next;
    }

    // Verify used + free memory equals total memory (minus initial header)
    assert(fl->used + free_mem == total_size);

    // Verify used memory is at least as large as the header
    assert(fl->used >= 0);

    // Verify used memory doesn't exceed total memory
    assert(fl->used <= total_size);
}

void
validate_freelist_memory(Freelist *fl, void *base_addr, size_t total_size)
{
    validate_freelist_order(fl);
    validate_used_memory(fl, base_addr, total_size);
}


void
freelist_realloc_tests()
{
    printf("Running realloc tests ...\n");

    // Initialize with 1MB of memory
    const size_t mem_size = 1024 * 1024;
    void *memory = malloc(mem_size);
    assert(memory != NULL);

    Freelist fl;
    fl.policy = PLACEMENT_POLICY_FIND_BEST;
    freelist_init(&fl, memory, mem_size, DEFAULT_ALIGNMENT);
    alloc_api *api = &fl.api;

    printf("1. Shrink down realloc test...\n");
    {
        size_t old_p1_size = 128;
        unsigned char *p1 = (unsigned char *)freelist_alloc(&fl, old_p1_size);
        size_t u1 = fl.used;
        for (unsigned char i = 0; i < 128; ++i) {
            p1[i] = i;
        }
        unsigned char *p2 = (unsigned char *)freelist_alloc(&fl, 256);
        size_t u2 = fl.used - u1;
        for (unsigned char i = 0; i < 255; ++i) {
            p2[i] = i;
        }
        unsigned char *p3 = (unsigned char *)freelist_alloc(&fl, 32);
        size_t u3 = fl.used - (u1+u2);
        for (unsigned char i = 0; i < 32; ++i) {
            p3[i] = i;
        }
        freelist_free(&fl, p2); p2 = NULL;
        uintptr_t old_head = (uintptr_t)fl.head;
        size_t new_p1_size = 120;
        unsigned char *new_p1 = (unsigned char *)freelist_realloc(&fl, p1, new_p1_size, DEFAULT_ALIGNMENT);
        uintptr_t new_head = (uintptr_t)fl.head;
        size_t p1_shrink_size = old_p1_size - new_p1_size;
        assert(old_head > new_head && (new_head + p1_shrink_size) == old_head);
        for (int i = 0; i < new_p1_size; ++i) {
            assert(p1[i] == i);
        }
        assert((uintptr_t)p1 == (uintptr_t)new_p1);
        Freelist_Node *first_free_node = fl.head;
        assert(first_free_node->block_size == u2+8);
        freelist_free(&fl, p1);
        freelist_free(&fl, p3);
        freelist_test_initial_state(&fl);

        unsigned char *ptr1 = (unsigned char *)freelist_alloc(&fl, 128);
        for (unsigned char i = 0; i < 128; ++i) {
            ptr1[i] = (i+1);
        }

        unsigned char *ptr2 = (unsigned char *)freelist_alloc(&fl, 512);
        memset(ptr2, 0x0a, 512);

        ptr1 = (unsigned char *)freelist_realloc(&fl, ptr1, 64, api->alignment);
        for (unsigned char i = 0; i < 64; ++i) {
            assert(ptr1[i] == (i+1));
        }

        assert(freelist_remaining_space(&fl) + fl.used == fl.size);
        freelist_free(&fl, ptr1);
        freelist_free(&fl, ptr2);
        freelist_test_initial_state(&fl);
    }

    printf("2. Basic realloc tests...\n");
    {
        // Test growing allocation
        void *ptr1 = freelist_alloc(&fl, 128);
        assert(ptr1 != NULL);
        memset(ptr1, 0xAA, 128);

        size_t used_before = fl.used;
        void *ptr2 = freelist_realloc(&fl, ptr1, 256, api->alignment);
        assert(ptr2 != NULL);
        // Verify content was preserved
        for (size_t i = 0; i < 128; i++) {
            assert(((unsigned char*)ptr2)[i] == 0xAA);
        }
        assert(fl.used >= used_before);
        validate_freelist_memory(&fl, memory, mem_size);

        // Test shrinking allocation
        used_before = fl.used;
        ptr1 = freelist_realloc(&fl, ptr2, 64, api->alignment);
        assert(ptr1 != NULL);
        // Verify content was preserved
        for (size_t i = 0; i < 64; i++) {
            assert(((unsigned char*)ptr1)[i] == 0xAA);
        }
        assert(fl.used <= used_before);
        validate_freelist_memory(&fl, memory, mem_size);

        freelist_free(&fl, ptr1);
        freelist_test_initial_state(&fl);
    }

    printf("3. Edge case tests...\n");
    {
        // NULL ptr (should behave like alloc)
        void *ptr1 = freelist_realloc(&fl, NULL, 128, api->alignment);
        assert(ptr1 != NULL);
        memset(ptr1, 0xBB, 128);
        validate_freelist_memory(&fl, memory, mem_size);

        // size 0 (should behave like free)
        size_t used_before = fl.used;
        void *ptr2 = freelist_realloc(&fl, ptr1, 0, api->alignment);
        assert(ptr2 == NULL);
        assert(fl.used < used_before);
        validate_freelist_memory(&fl, memory, mem_size);

        ptr1 = freelist_alloc(&fl, 256);
        assert(ptr1 != NULL);
        memset(ptr1, 0xCC, 256);
        used_before = fl.used;
        // with same size
        ptr2 = freelist_realloc(&fl, ptr1, 256, api->alignment);
        assert(ptr2 != NULL);
        assert(fl.used == used_before);
        for (size_t i = 0; i < 256; i++) {
            assert(((unsigned char*)ptr2)[i] == 0xCC);
        }
        validate_freelist_memory(&fl, memory, mem_size);
        freelist_free(&fl, ptr2);

        freelist_test_initial_state(&fl);
    }

    printf("4. Mixed allocation pattern tests...\n");
    {
        void *ptrs[10] = {NULL};
        size_t sizes[10] = {64, 128, 256, 512, 1024, 32, 96, 160, 384, 768};

        // Allocate with varying sizes
        for (int i = 0; i < 10; i++) {
            ptrs[i] = freelist_alloc(&fl, sizes[i]);
            assert(ptrs[i] != NULL);
            memset(ptrs[i], i+1, sizes[i]);
            validate_freelist_memory(&fl, memory, mem_size);
        }

        // Free some blocks to create holes
        freelist_free(&fl, ptrs[1]); ptrs[1] = NULL;
        freelist_free(&fl, ptrs[3]); ptrs[3] = NULL;
        freelist_free(&fl, ptrs[5]); ptrs[5] = NULL;
        freelist_free(&fl, ptrs[7]); ptrs[7] = NULL;
        validate_freelist_memory(&fl, memory, mem_size);

        // Reallocate remaining blocks with various sizes
        size_t new_sizes[10] = {32, 0, 512, 0, 2048, 0, 128, 0, 256, 1536};
        for (int i = 0; i < 10; i++) {
            if (ptrs[i] != NULL) {
                void *new_ptr = freelist_realloc(&fl, ptrs[i], new_sizes[i], api->alignment);
                if (new_sizes[i] > 0) {
                    assert(new_ptr != NULL);
                    ptrs[i] = new_ptr;
                } else {
                    assert(new_ptr == NULL);
                    ptrs[i] = NULL;
                }
                validate_freelist_memory(&fl, memory, mem_size);
            }
        }

        // Free remaining blocks
        for (int i = 0; i < 10; i++) {
            if (ptrs[i] != NULL) {
                freelist_free(&fl, ptrs[i]);
                validate_freelist_memory(&fl, memory, mem_size);
            }
        }

        freelist_test_initial_state(&fl);
    }

    printf("5. Basic realloc alignment tests...\n");
    {
        const size_t alignments[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
        void *ptrs[10] = {NULL};

        // Test growing allocation with different alignments
        for (int i = 0; i < 10; i++) {
            ptrs[i] = freelist_realloc(&fl, NULL, 128, alignments[i]);
            assert(ptrs[i] != NULL);
            assert((uintptr_t)ptrs[i] % alignments[i] == 0);
            memset(ptrs[i], 0xAA + i, 128);
            validate_freelist_memory(&fl, memory, mem_size);
        }

        // Grow allocations while maintaining alignment
        for (int i = 0; i < 10; i++) {
            void *new_ptr = freelist_realloc(&fl, ptrs[i], 256, alignments[i]);
            assert(new_ptr != NULL);
            assert((uintptr_t)new_ptr % alignments[i] == 0);
            // Verify content was preserved
            for (size_t j = 0; j < 128; j++) {
                assert(((unsigned char*)new_ptr)[j] == (0xAA + i));
            }
            ptrs[i] = new_ptr;
            validate_freelist_memory(&fl, memory, mem_size);
        }

        // Shrink allocations with different alignments
        for (int i = 0; i < 10; i++) {
            void *new_ptr = freelist_realloc(&fl, ptrs[i], 64, alignments[i]);
            assert(new_ptr != NULL);
            assert((uintptr_t)new_ptr % alignments[i] == 0);
            // Verify content was preserved
            for (size_t j = 0; j < 64; j++) {
                assert(((unsigned char*)new_ptr)[j] == (0xAA + i));
            }
            ptrs[i] = new_ptr;
            validate_freelist_memory(&fl, memory, mem_size);
        }

        // Free all allocations
        for (int i = 0; i < 10; i++) {
            freelist_free(&fl, ptrs[i]);
            validate_freelist_memory(&fl, memory, mem_size);
        }

        freelist_test_initial_state(&fl);
    }

    printf("6. Edge case alignment tests...\n");
    {
        // Test realloc with NULL pointer and alignment
        void *ptr1 = freelist_realloc(&fl, NULL, 128, 4096);
        assert(ptr1 != NULL);
        assert((uintptr_t)ptr1 % 4096 == 0);
        memset(ptr1, 0xDD, 128);
        validate_freelist_memory(&fl, memory, mem_size);

        // Test realloc with size 0 but large alignment
        void *ptr2 = freelist_realloc(&fl, ptr1, 0, 4096);
        assert(ptr2 == NULL);
        validate_freelist_memory(&fl, memory, mem_size);

        // Test realloc with same size but different alignment
        ptr1 = freelist_realloc(&fl, NULL, 256, 64);
        assert(ptr1 != NULL);
        assert((uintptr_t)ptr1 % 64 == 0);
        memset(ptr1, 0xEE, 256);

        ptr2 = freelist_realloc(&fl, ptr1, 256, 128);
        assert(ptr2 != NULL);
        assert((uintptr_t)ptr2 % 128 == 0);
        // Verify content
        for (size_t i = 0; i < 256; i++) {
            assert(((unsigned char*)ptr2)[i] == 0xEE);
        }

        freelist_free(&fl, ptr2);
        validate_freelist_memory(&fl, memory, mem_size);

        freelist_test_initial_state(&fl);
    }

    freelist_test_initial_state(&fl);
    freelist_free_all(&fl);
    freelist_test_initial_state(&fl);
    free(memory);
    printf("All realloc tests passed successfully!\n");
}

void
freelist_fragmentation_tests()
{
    // Setup 64MB memory pool with BEST FIT policy
    const size_t MEM_SIZE = 64 * 1024 * 1024; // 64 MB
    uint8_t *memory = malloc(MEM_SIZE);
    Freelist fl;
    freelist_init(&fl, memory, MEM_SIZE, DEFAULT_ALIGNMENT);
    fl.policy = PLACEMENT_POLICY_FIND_BEST;

    // Test 1: Create many small allocations followed by selective frees
    void *ptrs[1000];
    size_t sizes[1000];
    size_t block_sizes[1000];

    // Allocate blocks of varying sizes
    for (int i = 0; i < 1000; i++) {
        sizes[i] = 1024 + (i % 64) * 512; // Varying sizes from 1KB to 32KB
        ptrs[i] = freelist_alloc(&fl, sizes[i]);
        assert(ptrs[i] != NULL);
        block_sizes[i] = freelist_block_size(ptrs[i]);
    }

    // Free every third block to create fragmentation
    for (int i = 0; i < 1000; i += 3) {
        freelist_free(&fl, ptrs[i]);
    }

    // Test coalescing by tracking exact block sizes
    for (int i = 1; i < 999; i += 3) {
        size_t before_free = freelist_remaining_space(&fl);
        size_t block_i = freelist_block_size(ptrs[i]);
        freelist_free(&fl, ptrs[i]);
        assert(freelist_remaining_space(&fl) == before_free + block_i);

        size_t block_i_plus_1 = freelist_block_size(ptrs[i + 1]);
        freelist_free(&fl, ptrs[i + 1]);
        assert(freelist_remaining_space(&fl) == before_free + block_i + block_i_plus_1);
    }

    // Test 2: Test coalescing edge cases
    void *edge_ptrs[5];
    size_t edge_block_sizes[5];
    size_t block_size = 1024;

    // Allocate 5 consecutive blocks
    for (int i = 0; i < 5; i++) {
        edge_ptrs[i] = freelist_alloc(&fl, block_size);
        assert(edge_ptrs[i] != NULL);
        edge_block_sizes[i] = freelist_block_size(edge_ptrs[i]);
    }

    // Test coalescing patterns:
    // 1. Free middle block
    freelist_free(&fl, edge_ptrs[2]);
    size_t mid_free = freelist_remaining_space(&fl);

    // 2. Free left adjacent block - should coalesce
    size_t ptr_1_block_size = freelist_block_size(edge_ptrs[1]);
    freelist_free(&fl, edge_ptrs[1]);
    assert(freelist_remaining_space(&fl) == (mid_free + ptr_1_block_size));

    // 3. Free right adjacent block - should coalesce
    size_t ptr_3_block_size = freelist_block_size(edge_ptrs[3]);
    freelist_free(&fl, edge_ptrs[3]);
    assert(freelist_remaining_space(&fl) == (mid_free + ptr_1_block_size + ptr_3_block_size));

    // Free remaining edge pointers
    freelist_free(&fl, edge_ptrs[0]);
    freelist_free(&fl, edge_ptrs[4]);

    // Test 3: Stress realloc with accurate block size tracking
    void *realloc_ptr = freelist_alloc(&fl, 1024);
    assert(realloc_ptr != NULL);
    void *last_ptr = realloc_ptr;

    // Repeatedly grow and shrink, tracking actual block sizes
    for (int i = 0; i < 100; i++) {
        size_t current_block_size = freelist_block_size(realloc_ptr);
        size_t before_realloc = freelist_remaining_space(&fl);
        size_t new_size = 1024 * (1 + (i % 10));

        void *new_ptr = freelist_realloc(&fl, realloc_ptr, new_size, DEFAULT_ALIGNMENT);
        assert(new_ptr != NULL);
        last_ptr = new_ptr;

        size_t new_block_size = freelist_block_size(new_ptr);
        realloc_ptr = new_ptr;
    }

    freelist_free(&fl, last_ptr);

    // Test 4: Edge case - maximum size allocation
    void *max_ptr = freelist_alloc(&fl, MEM_SIZE - sizeof(Freelist_Allocation_Header));
    assert(max_ptr != NULL);                    // Should succeed as all memory is free and coalesced
    assert(freelist_remaining_space(&fl) == 0); // All memory should be allocated

    // Free max_ptr
    freelist_free(&fl, max_ptr);
    assert(freelist_remaining_space(&fl) == MEM_SIZE); // Should return to full free space

    // --------------------------------------------------------------------------------------------
    // Chaotic allocation test with prime sizes
    void *chaos_ptrs[500];
    size_t prime_sizes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};
    size_t num_primes = sizeof(prime_sizes) / sizeof(prime_sizes[0]);

    // Random allocations with prime-based sizes
    for (int i = 0; i < 500; i++) {
        size_t base = prime_sizes[i % num_primes];
        size_t size = base * 1023 + (i * 17) % 2048; // Weird sizes
        chaos_ptrs[i] = freelist_alloc(&fl, size);
        assert(chaos_ptrs[i] != NULL);
    }

    // Free in strange pattern: every prime index
    for (int i = 0; i < 500; i++) {
        for (int j = 0; j < num_primes; j++) {
            if (i == prime_sizes[j]) {
                freelist_free(&fl, chaos_ptrs[i]);
                chaos_ptrs[i] = NULL;
                break;
            }
        }
    }

    // Realloc every 7th remaining pointer to a size based on current free space
    for (int i = 0; i < 500; i += 7) {
        if (chaos_ptrs[i] != NULL) {
            size_t current_free = freelist_remaining_space(&fl);
            size_t new_size = (current_free % 4096)+1024; // Random but bounded size
            void *new_ptr = freelist_realloc(&fl, chaos_ptrs[i], new_size, DEFAULT_ALIGNMENT);
            assert(new_ptr != NULL);
            chaos_ptrs[i] = new_ptr;
        }
    }

    // Allocate tiny blocks between existing allocations
    void *tiny_ptrs[200];
    for (int i = 0; i < 200; i++) {
        tiny_ptrs[i] = freelist_alloc(&fl, (i%5)*8+16); // Tiny sizes: 16, 24, 32, 40, 48 bytes
    }

    // Free alternating blocks: odd indices from chaos_ptrs, even indices from tiny_ptrs
    for (int i = 1; i < 500; i += 2) {
        if (chaos_ptrs[i] != NULL) {
            freelist_free(&fl, chaos_ptrs[i]);
            chaos_ptrs[i] = NULL;
        }
    }

    for (int i = 0; i < 200; i += 2) {
        freelist_free(&fl, tiny_ptrs[i]);
        tiny_ptrs[i] = NULL;
    }

    // Try medium allocations to fill gaps
    void *medium_ptrs[50];
    for (int i = 0; i < 50; i++) {
        size_t size = 512 * (i % 7 + 1); // Sizes from 512 to 3584 in 512-byte steps
        medium_ptrs[i] = freelist_alloc(&fl, size);
    }

    // Free remaining blocks in reverse order
    for (int i = 199; i >= 0; i--) {
        if (i % 2 == 1)
        { // Free odd indices of tiny_ptrs
            freelist_free(&fl, tiny_ptrs[i]);
            tiny_ptrs[i] = NULL;
        }
    }

    for (int i = 499; i >= 0; i--) {
        if (chaos_ptrs[i] != NULL) {
            freelist_free(&fl, chaos_ptrs[i]);
            chaos_ptrs[i] = NULL;
        }
    }

    for (int i = 49; i >= 0; i--) {
        if (medium_ptrs[i] != NULL) {
            freelist_free(&fl, medium_ptrs[i]);
            medium_ptrs[i] = NULL;
        }
    }

    // Verify we can still allocate the maximum block
    max_ptr = freelist_alloc(&fl, MEM_SIZE - sizeof(Freelist_Allocation_Header));
    assert(max_ptr != NULL);
    freelist_free(&fl, max_ptr);

    // Verify all memory is recovered
    assert(freelist_remaining_space(&fl) == MEM_SIZE);
    // --------------------------------------------------------------------------------------------

    // Cleanup
    free(memory);
    printf("fragmentation tests passed successfully!");
}

void
freelist_alignment_tests()
{
    printf("freelist alignment tests!\n");

    // Initialize with 64MB of memory
    const size_t mem_size = 64 * 1024 * 1024;
    void *memory = malloc(mem_size);
    assert(memory != NULL);

    Freelist fl;
    fl.policy = PLACEMENT_POLICY_FIND_BEST;
    freelist_init(&fl, memory, mem_size, DEFAULT_ALIGNMENT);

    // Verify initial state
    assert(fl.head != NULL);
    assert((uintptr_t)fl.head >= (uintptr_t)memory);
    assert(fl.used == 0);
    validate_freelist_order(&fl);
    validate_used_memory(&fl, memory, mem_size);

    const size_t alloc_header_size = sizeof(Freelist_Allocation_Header);

    printf("1. Basic allocation tests with different alignments...\n");
    {
        // exhaust all the memory available
        size_t size = mem_size;
        void *ptr1 = freelist_alloc(&fl, size);
        // Not enough space for allocation header
        assert(ptr1 == NULL);
        size = mem_size - (sizeof(Freelist_Allocation_Header));
        ptr1 = freelist_alloc(&fl, size);
        // Allocates entire memory as header fits
        assert((ptr1 != NULL) && (fl.head == NULL) &&
               (fl.block_count == 0) && (fl.used == mem_size));
        freelist_free(&fl, ptr1);
        size_t remaining = freelist_remaining_space(&fl);
        assert(remaining == mem_size);
        size = mem_size - sizeof(Freelist_Allocation_Header) - 1;
        ptr1 = freelist_alloc(&fl, size);
        // 1 byte free but too small for headers, returns all memory
        assert((ptr1 != NULL) && (fl.head == NULL) &&
               (fl.block_count == 0) && (fl.used == mem_size));
        remaining = freelist_remaining_space(&fl);
        assert(remaining == 0);
        freelist_free(&fl, ptr1);
        remaining = freelist_remaining_space(&fl);
        assert(remaining == mem_size);
        size = mem_size - sizeof(Freelist_Allocation_Header) - (sizeof(Freelist_Node)+1);
        ptr1 = freelist_alloc(&fl, size);
        // Space left for free node, but only 1 usable byte due to header size
        remaining = freelist_remaining_space(&fl);
        assert((ptr1 != NULL) && (fl.head != NULL) && (fl.block_count == 1) &&
               (fl.used == size + sizeof(Freelist_Allocation_Header)));
        assert(remaining == sizeof(Freelist_Node)+1);
        // Realloc: Grows allocation by 1 byte
        ptr1 = freelist_realloc(&fl, ptr1, size+1, DEFAULT_ALIGNMENT);
        remaining = freelist_remaining_space(&fl);
        assert((ptr1 != NULL) && (fl.head != NULL) && (fl.block_count == 1) &&
               (fl.used == mem_size - sizeof(Freelist_Node)) &&
               (fl.head->block_size == sizeof(Freelist_Node)));
        // Realloc: Shrinks allocation back to original size
        ptr1 = freelist_realloc(&fl, ptr1, size, DEFAULT_ALIGNMENT);
        remaining = freelist_remaining_space(&fl);
        assert((ptr1 != NULL) && (fl.head != NULL) && (fl.block_count == 1) &&
               (fl.used == (mem_size - (sizeof(Freelist_Node)+1))) &&
               (fl.head->block_size == (sizeof(Freelist_Node)+1)));
        freelist_free(&fl, ptr1);
        remaining = freelist_remaining_space(&fl);
        assert(remaining == mem_size);
        void *ptr = freelist_alloc(&fl, 32);
        void *new_ptr = freelist_realloc(&fl, ptr, (mem_size+1), DEFAULT_ALIGNMENT);
        assert(new_ptr == NULL); // Should fail as size exceeds pool
        freelist_free_all(&fl);

        // Test allocations with various alignments
        const size_t alignments[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};
        void *ptrs[sizeof(alignments)/sizeof(alignments[0])] = {NULL};

        size_t total_allocated = 0;
        size_t expected_allocated = 0;
        // Pre-calculate expected total allocation
        for (int i = 0; i < sizeof(alignments)/sizeof(alignments[0]); ++i) {
            expected_allocated += 128; // alloc_size is fixed at 128 which is > sizeof(Freelist_Allocation_Header)
        }

        for (size_t i = 0; i < sizeof(alignments)/sizeof(alignments[0]); i++)
        {
            size_t alignment = alignments[i];
            size_t alloc_size = 128;

            size_t used_before = fl.used;
            ptrs[i] = freelist_alloc_align(&fl, alloc_size, alignment);
            assert(ptrs[i] != NULL);
            assert((uintptr_t)ptrs[i] % alignment == 0);

            Freelist_Allocation_Header *header = (Freelist_Allocation_Header *)((char *)ptrs[i] - alloc_header_size);
            assert((fl.used - used_before) == (header->block_size));
            size_t actual_alloc = (header->block_size - header->alignment_padding - alloc_header_size);
            assert(actual_alloc == alloc_size);
            total_allocated += actual_alloc;

            memset(ptrs[i], 0xAA, alloc_size);
            validate_freelist_order(&fl);
            validate_used_memory(&fl, memory, mem_size);

            for (size_t j = 0; j < i; j++) {
                if (ptrs[j] != NULL) {
                    assert((uintptr_t)ptrs[j] + 128 <= (uintptr_t)ptrs[i] ||
                        (uintptr_t)ptrs[i] + 128 <= (uintptr_t)ptrs[j]);
                }
            }
        }
        assert(total_allocated == expected_allocated);

        // Free all allocations in reverse order
        int alignment_count = sizeof(alignments) / sizeof(alignments[0]);
        for (int i = alignment_count-1; i >= 0; i--)
        {
            if (ptrs[i] != NULL) {
                size_t used_before = fl.used;
                Freelist_Allocation_Header *header =
                    (Freelist_Allocation_Header *)((char *)ptrs[i] - alloc_header_size);
                size_t block_size = header->block_size;

                freelist_free(&fl, ptrs[i]);
                assert(fl.used == used_before - block_size);
                validate_freelist_order(&fl);
                validate_used_memory(&fl, memory, mem_size);
            }
        }
    }


    freelist_free_all(&fl);
    printf("2. Mixed size and alignment tests...\n");
    {
        struct {
            size_t size;
            size_t alignment;
        } test_cases[] = {
            {64, 8},
            {256, 32},
            {1024, 128},
            {4096, 4096},
            {7, 8},
            {1000, 64},
            {64, 256},
            {2048, 16}
        };

        void *ptrs[sizeof(test_cases)/sizeof(test_cases[0])] = {NULL};

        size_t total_allocated_minus_padding = 0;
        size_t expected_allocated = 0;
        for (int i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); ++i) {
            expected_allocated += test_cases[i].size > sizeof(Freelist_Node) ? test_cases[i].size
                                                                             : sizeof(Freelist_Node);
        }

        for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
            size_t alloc_size = test_cases[i].size;
            size_t alignment = test_cases[i].alignment;

            size_t used_before = fl.used;
            ptrs[i] = freelist_alloc_align(&fl, alloc_size, alignment);
            assert(ptrs[i] != NULL);
            assert((uintptr_t)ptrs[i] % alignment == 0);

            Freelist_Allocation_Header *header = (Freelist_Allocation_Header *)((char *)ptrs[i] - alloc_header_size);
            assert((fl.used - used_before) == (header->block_size));
            size_t actual_alloc = (header->block_size - header->alignment_padding - alloc_header_size);
            assert(actual_alloc == alloc_size || actual_alloc == sizeof(Freelist_Node));
            total_allocated_minus_padding += actual_alloc;

            // Fill allocated memory with pattern
            memset(ptrs[i], 0xBB + i, alloc_size);

            validate_freelist_order(&fl);
            validate_used_memory(&fl, memory, mem_size);
        }
        assert(total_allocated_minus_padding == expected_allocated);
        assert(fl.block_count == 1);
        // Free in mixed order to test coalescing
        size_t free_order[] = {0, 2, 4, 1, 6, 3, 5, 7};
        int exp_block_counts[] = {2, 3, 4, 3, 4, 3, 2, 1};
        size_t exp_free_blocks[8][4] = {
                                            {80,   0,    0,   0},
                                            {80,   1056, 0,   0},
                                            {80,   1056, 32,  0},
                                            {1424, 32,   0,   0},
                                            {1424, 32,   280, 0},
                                            {8112, 280,  0,   0},
                                            {9424, 0,    0,   0},
                                            {0,    0,    0,   0},
                                       };
        for (size_t i = 0; i < sizeof(free_order)/sizeof(free_order[0]); i++)
        {
            size_t idx = free_order[i];
            size_t size = test_cases[idx].size;
            size_t size_exp = size < sizeof(Freelist_Node) ? sizeof(Freelist_Node) : size;
            size_t alignment = test_cases[idx].alignment;

            Freelist_Allocation_Header *header = (Freelist_Allocation_Header *)((char *)ptrs[idx] -
                                                                                sizeof(Freelist_Allocation_Header));
            size_t allocation_block_size = header->block_size;
            // Calculate the total size that should be freed
            total_allocated_minus_padding -= (header->block_size -
                                              (header->alignment_padding + sizeof(Freelist_Allocation_Header)));
            assert((header->block_size - header->alignment_padding - sizeof(Freelist_Allocation_Header)) == size_exp);

            size_t used_before = fl.used;
            freelist_free(&fl, ptrs[idx]);
            assert(fl.used == used_before - allocation_block_size);

            assert(exp_block_counts[i] == fl.block_count);
            verify_freelist_blocks(&fl, exp_free_blocks[i], exp_block_counts[i], false);

            validate_freelist_order(&fl);
            validate_used_memory(&fl, memory, mem_size);
        }
        assert((total_allocated_minus_padding == 0));
    }

    assert((fl.block_count == 1) && (fl.used == 0) && (fl.head->block_size == mem_size));
    freelist_free_all(&fl);
    printf("3. Alignment stress tests...\n");
    {
        // Test worst-case alignment scenarios
        const size_t max_alignment = 4096;
        size_t used_before = fl.used;

        void *ptr1 = freelist_alloc_align(&fl, 1, max_alignment);
        assert(ptr1 != NULL);
        assert((uintptr_t)ptr1 % max_alignment == 0);
        Freelist_Allocation_Header *header1 = (Freelist_Allocation_Header *)((char *)ptr1 - alloc_header_size);
        assert((fl.used - used_before) == header1->block_size);
        size_t actual_alloc1 = (header1->block_size - header1->alignment_padding - alloc_header_size);
        assert(actual_alloc1 == sizeof(Freelist_Node)); // Minimum allocation size

        void *ptr2 = freelist_alloc_align(&fl, 1, 8);
        assert(ptr2 != NULL);
        assert((uintptr_t)ptr2 % 8 == 0);
        Freelist_Allocation_Header *header2 = (Freelist_Allocation_Header *)((char *)ptr2 - alloc_header_size);
        assert((fl.used - (used_before + header1->block_size)) == header2->block_size);
        size_t actual_alloc2 = (header2->block_size - header2->alignment_padding - alloc_header_size);
        assert(actual_alloc2 == sizeof(Freelist_Node)); // Minimum allocation size

        void *ptr3 = freelist_alloc_align(&fl, 1, max_alignment);
        assert(ptr3 != NULL);
        assert((uintptr_t)ptr3 % max_alignment == 0);
        Freelist_Allocation_Header *header3 = (Freelist_Allocation_Header *)((char *)ptr3 - alloc_header_size);
        size_t actual_alloc3 = (header3->block_size - header3->alignment_padding - alloc_header_size);
        assert(actual_alloc3 == sizeof(Freelist_Node)); // Minimum allocation size

        // Verify no overlap
        if (ptr1 < ptr2) {
            assert((uintptr_t)ptr1 + sizeof(Freelist_Node) <= (uintptr_t)ptr2);
        } else {
            assert((uintptr_t)ptr2 + sizeof(Freelist_Node) <= (uintptr_t)ptr1);
        }
        if (ptr2 < ptr3) {
            assert((uintptr_t)ptr2 + sizeof(Freelist_Node) <= (uintptr_t)ptr3);
        } else {
            assert((uintptr_t)ptr3 + sizeof(Freelist_Node) <= (uintptr_t)ptr2);
        }

        freelist_free(&fl, ptr1);
        freelist_free(&fl, ptr2);
        freelist_free(&fl, ptr3);

        assert(fl.used == used_before);
        validate_freelist_order(&fl);
        validate_used_memory(&fl, memory, mem_size);
    }

    assert((fl.block_count == 1) && (fl.used == 0) && (fl.head->block_size == mem_size));
    free(memory);
    printf("All alignment tests passed successfully!\n");
}

void
freelist_unit_tests()
{
    freelist_alignment_tests();
    freelist_realloc_tests();
    freelist_fragmentation_tests();
}
#endif
#endif
#endif