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
struct Freelist {
    void *data;
    size_t size;
    size_t used;
    Freelist_Node *head;
    Placement_Policy policy;
    int block_count;
};

void  freelist_init(Freelist *fl, void *data, size_t size);
void *freelist_alloc(Freelist *fl, size_t size, size_t alignment);
void  freelist_free(Freelist *fl, void *ptr);
void *freelist_realloc(Freelist *fl, void *ptr, size_t new_size, size_t alignment);
void  freelist_free_all(Freelist *fl);

#ifdef FREELIST_ALLOCATOR_UNIT_TESTS
void freelist_tests();
#endif

#ifdef FREELIST_ALLOCATOR_IMPLEMENTATION
void  freelist_node_insert(Freelist *fl, Freelist_Node *prev_node, Freelist_Node *new_node);
void  freelist_node_remove(Freelist *fl, Freelist_Node *prev_node, Freelist_Node *del_node);
void *freelist_realloc_sized(Freelist *fl, void *ptr, size_t old_size, size_t new_size, size_t alignment);

void
freelist_free_all(Freelist *fl)
{
    fl->used = 0;
    Freelist_Node *first_node = (Freelist_Node *)fl->data;
    first_node->block_size = fl->size;
    first_node->next = NULL;
    fl->head = first_node;
    fl->block_count = 1;
}

void
freelist_init(Freelist *fl, void *data, size_t size)
{
    fl->data = data;
    fl->size = size;
    freelist_free_all(fl);
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
freelist_alloc(Freelist *fl, size_t size, size_t alignment)
{
    size_t padding = 0;
    Freelist_Node *prev_node = NULL;
    Freelist_Node *node = NULL;

    size_t alignment_padding, required_space, remaining;
    Freelist_Allocation_Header *header_ptr;
    size_t header_size = sizeof(Freelist_Allocation_Header);

    if (size < sizeof(Freelist_Node)) size = sizeof(Freelist_Node);
    if (alignment < 8) alignment = 8;

    if (fl->policy == PLACEMENT_POLICY_FIND_BEST) {
        node = freelist_find_best(fl, size, alignment, &padding, &prev_node);
    } else if (fl->policy == PLACEMENT_POLICY_FIND_FIRST) {
        node = freelist_find_first(fl, size, alignment, &padding, &prev_node);
    } else {
        assert(0 && "No Valid placement policy set!");
    }

    if (node == NULL) {
        assert(0 && "Freelist has no memory");
        return NULL;
    }

    assert(padding >= header_size);
    alignment_padding = padding - header_size;

    required_space = size + padding;
    assert(node->block_size >= required_space);
    remaining = node->block_size - required_space;

    if (remaining > 0) {
        // a free node should have more than memory than is required to store the header.
        if (remaining > header_size) {
            Freelist_Node *new_node = (Freelist_Node *)((char *)node + required_space);
            new_node->block_size = remaining;
            freelist_node_insert(fl, node, new_node);
        } else {
            required_space += remaining;
        }
    }

    freelist_node_remove(fl, prev_node, node);

    header_ptr = (Freelist_Allocation_Header *)((char *)node + alignment_padding);
    header_ptr->block_size = required_space;
    header_ptr->alignment_padding = alignment_padding;

    fl->used += required_space;

    void *memory = (void *)((char *)header_ptr + header_size);
    return memory;
}

void freelist_coalescence(Freelist *fl, Freelist_Node *prev_node, Freelist_Node *free_node);

void
freelist_free(Freelist *fl, void *ptr)
{
    Freelist_Allocation_Header *header;
    Freelist_Node *free_node, *node, *prev_node = NULL;

    if (ptr == NULL) {
        return;
    }

    header = (Freelist_Allocation_Header *)((char *)ptr - sizeof(Freelist_Allocation_Header));

    // the actual header comes after padding.
    free_node = (Freelist_Node *)((char *)header - header->alignment_padding);
    free_node->block_size = header->block_size;
    free_node->next = NULL;
    assert(fl->used >= free_node->block_size);

    node = fl->head;
    while (node != NULL) {
        if (ptr < (void *)node) {
            freelist_node_insert(fl, prev_node, free_node);
            break;
        }
        prev_node = node;
        node = node->next;
    }
    fl->used -= free_node->block_size;
    freelist_coalescence(fl, prev_node, free_node);
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
freelist_realloc(Freelist *fl, void *ptr, size_t new_size, size_t alignment)
{
    assert(ptr != NULL);
    size_t header_size = sizeof(Freelist_Allocation_Header);
    Freelist_Allocation_Header *header = (Freelist_Allocation_Header *)((char *)ptr - header_size);

    size_t header_align_padding = header_size + header->alignment_padding;
    size_t old_size             = header->block_size - header_align_padding;
    new_size                    = (new_size > header_size) ? new_size : header_size;

    return freelist_realloc_sized(fl, ptr, old_size, new_size, alignment);
}

void *
freelist_realloc_sized(Freelist *fl, void *ptr, size_t old_size, size_t new_size, size_t alignment)
{
    assert(fl != NULL && ptr != NULL);

    if (old_size == new_size) {
        return ptr;
    }

    Freelist_Allocation_Header *header = (Freelist_Allocation_Header *)((char *)ptr -
                                                                        sizeof(Freelist_Allocation_Header));

    if (old_size < new_size)
    {
        const size_t extra_space = new_size - old_size;

        for (Freelist_Node *curr = fl->head, *prev = NULL;
             curr != NULL;
             prev = curr, curr = curr->next)
        {
            if ((uintptr_t)curr == ((uintptr_t)ptr + old_size) &&
                curr->block_size >= extra_space)
            {
                header->block_size += extra_space;

                const size_t remaining = curr->block_size - extra_space;
                if (remaining >= sizeof(Freelist_Allocation_Header))
                {
                    Freelist_Node *new_node = (Freelist_Node *)((char *)curr + extra_space);
                    new_node->next = curr->next;
                    new_node->block_size = remaining;

                    if (prev) {
                        prev->next = new_node;
                    } else {
                        fl->head = new_node;
                    }
                } else {
                    freelist_node_remove(fl, prev, curr);
                }
                return ptr;
            }
        }

        void *new_ptr = freelist_alloc(fl, new_size, alignment);
        if (new_ptr) {
            memcpy(new_ptr, ptr, old_size);
            freelist_free(fl, ptr);
            return new_ptr;
        }
        return NULL;
    }

    const size_t free_space = old_size - new_size;
    assert(free_space < sizeof(Freelist_Allocation_Header));

    header->block_size = new_size;

    Freelist_Node *new_node = (Freelist_Node *)((char *)ptr + new_size);
    new_node->block_size = free_space;
    new_node->next = NULL;

    Freelist_Node *curr = fl->head;
    Freelist_Node *prev = NULL;

    while (curr && (uintptr_t)curr < (uintptr_t)new_node) {
        prev = curr;
        curr = curr->next;
    }

    freelist_node_insert(fl, prev, new_node);

    if (new_node->next &&
        (char *)new_node + new_node->block_size == (char *)new_node->next)
    {
        new_node->block_size += new_node->next->block_size;
        freelist_node_remove(fl, new_node, new_node->next);
    }

    if (prev &&
        (char *)prev + prev->block_size == (char *)new_node)
    {
        prev->block_size += new_node->block_size;
        freelist_node_remove(fl, prev, new_node);
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
    assert(fl->block_count > 0);
}

#ifdef FREELIST_ALLOCATOR_UNIT_TESTS
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
freelist_alignment_tests()
{
    printf("freelist alignment tests!\n");

    // Initialize with 64MB of memory
    const size_t mem_size = 64 * 1024 * 1024;
    void *memory = malloc(mem_size);
    assert(memory != NULL);

    Freelist fl;
    fl.policy = PLACEMENT_POLICY_FIND_BEST;
    freelist_init(&fl, memory, mem_size);

    // Verify initial state
    assert(fl.head != NULL);
    assert((uintptr_t)fl.head >= (uintptr_t)memory);
    assert(fl.used == 0);
    validate_freelist_order(&fl);
    validate_used_memory(&fl, memory, mem_size);

    const size_t header_size = sizeof(Freelist_Allocation_Header);

    printf("1. Basic allocation tests with different alignments...\n");
    {
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
            ptrs[i] = freelist_alloc(&fl, alloc_size, alignment);
            assert(ptrs[i] != NULL);
            assert((uintptr_t)ptrs[i] % alignment == 0);

            Freelist_Allocation_Header *header = (Freelist_Allocation_Header *)((char *)ptrs[i] - header_size);
            assert((fl.used - used_before) == (header->block_size));
            size_t actual_alloc = (header->block_size - header->alignment_padding - header_size);
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
        for (int i = sizeof(alignments)/sizeof(alignments[0]) - 1; i >= 0; i--) {
            if (ptrs[i] != NULL) {
                size_t used_before = fl.used;
                Freelist_Allocation_Header *header =
                    (Freelist_Allocation_Header *)((char *)ptrs[i] - header_size);
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
            ptrs[i] = freelist_alloc(&fl, alloc_size, alignment);
            assert(ptrs[i] != NULL);
            assert((uintptr_t)ptrs[i] % alignment == 0);

            Freelist_Allocation_Header *header = (Freelist_Allocation_Header *)((char *)ptrs[i] - header_size);
            assert((fl.used - used_before) == (header->block_size));
            size_t actual_alloc = (header->block_size - header->alignment_padding - header_size);
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

        void *ptr1 = freelist_alloc(&fl, 1, max_alignment);
        assert(ptr1 != NULL);
        assert((uintptr_t)ptr1 % max_alignment == 0);
        Freelist_Allocation_Header *header1 = (Freelist_Allocation_Header *)((char *)ptr1 - header_size);
        assert((fl.used - used_before) == header1->block_size);
        size_t actual_alloc1 = (header1->block_size - header1->alignment_padding - header_size);
        assert(actual_alloc1 == sizeof(Freelist_Node)); // Minimum allocation size

        void *ptr2 = freelist_alloc(&fl, 1, 8);
        assert(ptr2 != NULL);
        assert((uintptr_t)ptr2 % 8 == 0);
        Freelist_Allocation_Header *header2 = (Freelist_Allocation_Header *)((char *)ptr2 - header_size);
        assert((fl.used - (used_before + header1->block_size)) == header2->block_size);
        size_t actual_alloc2 = (header2->block_size - header2->alignment_padding - header_size);
        assert(actual_alloc2 == sizeof(Freelist_Node)); // Minimum allocation size

        void *ptr3 = freelist_alloc(&fl, 1, max_alignment);
        assert(ptr3 != NULL);
        assert((uintptr_t)ptr3 % max_alignment == 0);
        Freelist_Allocation_Header *header3 = (Freelist_Allocation_Header *)((char *)ptr3 - header_size);
        size_t actual_alloc3 = (header3->block_size - header3->alignment_padding - header_size);
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
freelist_tests()
{
    freelist_alignment_tests();
}
#endif
#endif
#endif