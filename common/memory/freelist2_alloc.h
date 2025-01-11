#ifndef FREELIST2_ALLOC_H
#define FREELIST2_ALLOC_H

#include "memory.h"
#include <stdint.h>
#include <containers/htable.h>

typedef struct Freelist2_Allocation_Header {
    /// @brief this includes required size, size for header and size for alignment padding.
    size_t block_size;
    /// @brief size for alignment only
    size_t alignment_padding;
    /// @brief
    size_t alignment_requirement;
    /// @brief pointer to the owner of allocated memory
    void **pOwner;
} Freelist2_Allocation_Header;

typedef struct Freelist2_Node {
    struct Freelist2_Node *next;
    size_t block_size;
} Freelist2_Node;

typedef struct Freelist2 {
    alloc_api api;

    void *data;
    size_t size;
    size_t used;
    Freelist2_Node *head;

    htable_ptr_ptr table;
    Placement_Policy policy;
    int block_count;
} Freelist2;

void        freelist2_init(Freelist2 *fl, void *data, size_t size, size_t alignment);
alloc_api  *freelist2_get_api(Freelist2 *fl);
void       *freelist2_alloc(void *fl, size_t size);
void       *freelist2_alloc_align(void *fl, size_t size, size_t alignment);
void        freelist2_free(void *fl, void *ptr);
void       *freelist2_realloc(void *fl, void *ptr, size_t new_size);
void        freelist2_defragment(Freelist2 *fl);
bool        freelist2_set_allocation_owner(Freelist2 *fl, void *ptr, void **owner);

void        freelist2_free_all(void *fl);
size_t      freelist2_remaining_space(Freelist2 *fl);

#define SHALLOC_MANAGED(FL, sz, owner, type)                                                                      \
    type *owner = freelist2_alloc(&FL, 32);                                                                       \
    freelist2_set_allocation_owner(&FL, owner, (void **)&owner)
#define SHFREE_MANAGED(FL, ptr)                                                                                   \
    do                                                                                                            \
    {                                                                                                             \
        freelist2_free(&FL, ptr);                                                                                 \
        ptr = NULL;                                                                                               \
    } while (0)

#ifdef FREELIST2_ALLOCATOR_UNIT_TESTS
void freelist2_unit_tests();
#endif

#ifdef FREELIST2_ALLOCATOR_IMPLEMENTATION
#include <stdio.h>
void  freelist2_node_insert(Freelist2 *fl, Freelist2_Node *prev_node, Freelist2_Node *new_node);
void  freelist2_node_remove(Freelist2 *fl, Freelist2_Node *prev_node, Freelist2_Node *del_node);
void *freelist2_realloc_sized(void *fl, void *ptr, size_t old_size, size_t new_size, size_t alignment);
size_t freelist2_block_size(void *ptr);
Freelist2_Node *freelist2_find_first(Freelist2 *fl, size_t size, size_t alignment, size_t *padding_,
                                     Freelist2_Node **prev_node_);
Freelist2_Node *freelist2_find_best(Freelist2 *fl, size_t size, size_t alignment, size_t *padding_,
                                    Freelist2_Node **prev_node_);

static Freelist2_Allocation_Header *
freelist2_get_header(void *allocation)
{
    Freelist2_Allocation_Header *header = (Freelist2_Allocation_Header *)((uintptr_t)allocation -
                                                                        sizeof(Freelist2_Allocation_Header));
    return header;
}

bool
freelist2_set_allocation_owner(Freelist2 *fl, void *allocation, void **pOwner)
{
    assert(fl && pOwner);
    if (allocation == NULL) {
        return false;
    }

    Freelist2_Allocation_Header *header = freelist2_get_header(allocation);

    size_t metadata_size = (header->alignment_padding + sizeof(Freelist2_Allocation_Header));
    assert((uintptr_t)allocation > metadata_size);
    uintptr_t start_boundary = (uintptr_t)allocation - metadata_size;

    assert(start_boundary >= (uintptr_t)fl->data && start_boundary < ((uintptr_t)fl->data + fl->size));

    uintptr_t val = 0;
    bool exists = htget(ptr_ptr, &fl->table, start_boundary, &val);
    assert(exists && val == (uintptr_t)allocation);

    header->pOwner = pOwner;
    return exists;
}

size_t freelist2_block_size(void *ptr) {
    assert(ptr != NULL);
    Freelist2_Allocation_Header *header = freelist2_get_header(ptr);
    size_t result = header->block_size;
    return result;
}

size_t
freelist2_remaining_space(Freelist2 *fl)
{
    Freelist2_Node *curr = fl->head;
    size_t space = 0;
    while (curr != NULL) {
        space += curr->block_size;
        curr = curr->next;
    }
    assert(space == fl->size - fl->used);
    return space;
}

void
freelist2_free_all(void *fl)
{
    Freelist2 *freelist = (Freelist2 *)fl;
    freelist->used = 0;
    Freelist2_Node *first_node = (Freelist2_Node *)freelist->data;
    first_node->block_size = freelist->size;
    first_node->next = NULL;
    freelist->head = first_node;
    freelist->block_count = 1;
}

void
freelist2_init(Freelist2 *fl, void *data, size_t size, size_t alignment)
{
    fl->data = data;
    fl->size = size;
    freelist2_free_all(fl);
    fl->policy = PLACEMENT_POLICY_FIND_BEST;

    htinit(ptr_ptr, &fl->table, 1024, .2f, .7f, 217, NULL);

    fl->api.alloc = freelist2_alloc;
    fl->api.alloc_align = freelist2_alloc_align;
    // fl->api.realloc_align = freelist2_realloc;
    fl->api.free = freelist2_free;
    fl->api.free_all = freelist2_free_all;

    fl->api.alignment = alignment;
    fl->api.allocator = (void *)fl;
}

alloc_api *
freelist2_get_api(Freelist2 *fl)
{
    alloc_api *api = &fl->api;
    return api;
}

Freelist2_Node *
freelist2_find_first(Freelist2 *fl, size_t size, size_t alignment, size_t *padding_, Freelist2_Node **prev_node_)
{
    Freelist2_Node *node = fl->head;
    Freelist2_Node *prev_node = NULL;

    size_t padding = 0;

    while(node != NULL) {
        padding = calc_padding_with_header((uintptr_t)node, alignment, sizeof(Freelist2_Allocation_Header));
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

Freelist2_Node *
freelist2_find_best(Freelist2 *fl, size_t size, size_t alignment, size_t *padding_, Freelist2_Node **prev_node_)
{
    size_t smallest_diff = ~(size_t)0;

    Freelist2_Node *node = fl->head,
                  *prev_node = NULL,
                  *best_node = NULL;

    size_t padding = 0;
    while (node != NULL) {
        padding = calc_padding_with_header((uintptr_t)node, (uintptr_t)alignment,
                                           sizeof(Freelist2_Allocation_Header));
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
freelist2_alloc(void *fl, size_t size)
{
    return freelist2_alloc_align(fl, size, DEFAULT_ALIGNMENT);
}

void *
freelist2_alloc_align(void *fl, size_t size, size_t alignment)
{
    assert(fl != NULL);
    Freelist2 *freelist = (Freelist2 *)fl;

    size_t padding = 0;
    Freelist2_Node *prev_node = NULL;
    Freelist2_Node *node = NULL;

    size_t alignment_padding, required_space, remaining;
    Freelist2_Allocation_Header *header_ptr;
    size_t alloc_header_size = sizeof(Freelist2_Allocation_Header);

    // we have to save info about the free node which this alloc will be when we free it. So we need enough space
    // to store free_node info.
    if (size < sizeof(Freelist2_Node)) size = sizeof(Freelist2_Node);
    if (alignment < 8) alignment = 8;

    if (freelist->policy == PLACEMENT_POLICY_FIND_BEST) {
        node = freelist2_find_best(freelist, size, alignment, &padding, &prev_node);
    } else if (freelist->policy == PLACEMENT_POLICY_FIND_FIRST) {
        node = freelist2_find_first(freelist, size, alignment, &padding, &prev_node);
    } else {
        assert(0 && "No Valid placement policy set!");
    }

    if (node == NULL) {
        printf("Freelist2 does not have enough memory.\n");
        return NULL;
    }

    assert(padding >= alloc_header_size);
    alignment_padding = padding - alloc_header_size;

    required_space = size + padding;
    assert(node->block_size >= required_space);
    remaining = node->block_size - required_space;

    if (remaining > 0) {
        // a free node should have more than memory than is required to store header(metadata information).
        if (remaining > sizeof(Freelist2_Allocation_Header) &&
            remaining > sizeof(Freelist2_Node))
        {
            Freelist2_Node *new_node = (Freelist2_Node *)((uintptr_t)node + required_space);
            new_node->block_size = remaining;
            freelist2_node_insert(freelist, node, new_node);
        } else {
            // since we can't use the remaining space effectively, we just add to the allocated block--the
            // remaining space.
            required_space += remaining;
        }
    }

    freelist2_node_remove(freelist, prev_node, node);

    header_ptr = (Freelist2_Allocation_Header *)((uintptr_t)node + alignment_padding);
    memset(header_ptr, 0, sizeof(Freelist2_Allocation_Header));

    header_ptr->block_size = required_space;
    header_ptr->alignment_padding = alignment_padding;
    header_ptr->alignment_requirement = alignment;

    freelist->used += required_space;

    void *memory = (void *)((uintptr_t)header_ptr + alloc_header_size);
    htpush(ptr_ptr, &freelist->table, (uintptr_t)node, (uintptr_t)memory);
    return memory;
}

void freelist_merge_blocks_if_adjacent(Freelist2 *fl, Freelist2_Node *prev_node, Freelist2_Node *free_node);

void
freelist2_free(void *fl, void *ptr)
{
    assert(fl != NULL && ptr != NULL);
    Freelist2 *freelist = (Freelist2 *)fl;

    Freelist2_Allocation_Header *header;
    Freelist2_Node *free_node = NULL;

    if (ptr == NULL) {
        return;
    }

    header = freelist2_get_header(ptr);

    // the actual header comes after padding.
    free_node = (Freelist2_Node *)((uintptr_t)header - header->alignment_padding);
    free_node->block_size = header->block_size;
    free_node->next = NULL;

    htdel(ptr_ptr, &freelist->table, (uintptr_t)free_node);

    assert(freelist->used >= free_node->block_size);
    freelist->used -= free_node->block_size;
    if (freelist->head != NULL) {
        Freelist2_Node *node = freelist->head;
        Freelist2_Node *prev_node = NULL;
        while (node != NULL) {
            if (ptr < (void *)node) {
                freelist2_node_insert(freelist, prev_node, free_node);
                break;
            }
            prev_node = node;
            node = node->next;
        }
        freelist_merge_blocks_if_adjacent(freelist, prev_node, free_node);
    } else {
        freelist->head = free_node;
        freelist->block_count = 1;
    }
}

void
freelist_merge_blocks_if_adjacent(Freelist2 *fl, Freelist2_Node *prev_node, Freelist2_Node *free_node)
{
    if ((free_node->next != NULL) &&
        (void *)((uintptr_t)free_node + free_node->block_size) == free_node->next)
    {
        free_node->block_size += free_node->next->block_size;
        assert(fl->block_count >= 2);
        freelist2_node_remove(fl, free_node, free_node->next);
    }

    if ((prev_node != NULL && prev_node->next != NULL) &&
        (void *)((uintptr_t)prev_node + prev_node->block_size) == free_node)
    {
        prev_node->block_size += free_node->block_size;
        assert(fl->block_count >= 2);
        freelist2_node_remove(fl, prev_node, free_node);
    }
    assert(fl->block_count > 0);
}

void *
freelist2_realloc(void *fl, void *ptr, size_t new_size)
{
    assert(fl != NULL);
    if (ptr == NULL) {
        // should behave like alloc.
        return freelist2_alloc(fl, new_size);
    }
    if (new_size == 0) {
        freelist2_free(fl, ptr);
        return NULL;
    }

    Freelist2 *freelist = (Freelist2 *)fl;
    if ((uintptr_t)ptr < (uintptr_t)(freelist->data) ||
        (uintptr_t)ptr > ((uintptr_t)freelist->data + (uintptr_t)freelist->size))
    {
        printf("Out of Bounds Memory Access!\n");
        return NULL;
    }

    Freelist2_Allocation_Header *header = freelist2_get_header(ptr);
    if ((uintptr_t)ptr % header->alignment_requirement != 0) {
        printf("Misaligned memory!\n");
        return NULL;
    }
    size_t header_align_padding = sizeof(Freelist2_Allocation_Header) + header->alignment_padding;
    size_t old_size             = header->block_size - header_align_padding;
    if (old_size == new_size) {
        return ptr;
    }
    size_t remaining_space = freelist2_remaining_space(freelist);
    if (new_size > old_size) {
        assert(remaining_space >= sizeof(Freelist2_Allocation_Header));
    }
    if ((new_size > old_size) &&
        (new_size - old_size) > (remaining_space - sizeof(Freelist2_Allocation_Header)))
    {
        printf("Not enough space in the backing buffer!\n");
        return NULL;
    }

    void *new_ptr = freelist2_realloc_sized(fl, ptr, old_size, new_size, header->alignment_requirement);

    assert((freelist2_remaining_space(freelist) + freelist->used) == freelist->size);
    return new_ptr;
}

void *
freelist2_realloc_sized(void *fl, void *ptr, size_t old_size, size_t new_size, size_t alignment)
{
    assert(fl != NULL && ptr != NULL);

    Freelist2 *freelist = (Freelist2 *)fl;
    Freelist2_Allocation_Header *alloc_header = freelist2_get_header(ptr);
    if (old_size < new_size)
    {
        size_t extra_space = new_size - old_size;
        for (Freelist2_Node *curr = freelist->head, *prev = NULL;
             curr != NULL;
             prev = curr, curr = curr->next)
        {
            if ((uintptr_t)curr == ((uintptr_t)ptr + old_size) &&
                (curr->block_size >= extra_space))
            {
                const size_t remaining = curr->block_size - extra_space;
                if (remaining > sizeof(Freelist2_Allocation_Header) &&
                    remaining > sizeof(Freelist2_Node))
                {
                    Freelist2_Node *new_node = (Freelist2_Node *)((uintptr_t)curr + extra_space);
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
                    freelist2_node_remove(freelist, prev, curr);
                }
                alloc_header->block_size += extra_space;
                freelist->used += extra_space;
                return ptr;
            }
        }

        // did not find an immediate contiguous freeblock fitting the new size, allocating an entirely new memory
        // block and copyign the old data there.
        void **original_owner = alloc_header->pOwner;
        void *new_memory_ptr = freelist2_alloc_align(freelist, new_size, alignment);
        if (new_memory_ptr) {
            // TODO: Make a fast version of memcpy/memmove etc.
            memcpy(new_memory_ptr, ptr, old_size);
            freelist2_free(freelist, ptr);

            if (original_owner != NULL) {
                Freelist2_Allocation_Header *header = freelist2_get_header(new_memory_ptr);
                // copy over the address of the original memory owner to this new header.
                header->pOwner = original_owner;
                // make the original allocation owner point to this new memory that was allocated.
                *header->pOwner = new_memory_ptr;
            }
            return new_memory_ptr;
        }

        assert(new_memory_ptr == NULL);
        return NULL;
    }

    assert(old_size > new_size);
    const size_t free_space = old_size - new_size;

    if (free_space < sizeof(Freelist2_Node)) {
        // cannot make a free node just containing free_space.
        // checking if there is a free block immediately after old allocation.
        Freelist2_Node *curr = freelist->head;
        Freelist2_Node *prev = NULL;
        while (curr != NULL && (uintptr_t)curr < (uintptr_t)ptr) {
            prev = curr;
            curr = curr->next;
        }
        uintptr_t addr_after_allocation = (uintptr_t)ptr + old_size;
        if ((uintptr_t)curr == (uintptr_t)(addr_after_allocation)) {
            // lining up
            size_t new_block_size = curr->block_size + free_space;
            Freelist2_Node *new_node = (Freelist2_Node *)(addr_after_allocation - free_space);
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

    Freelist2_Node *new_node = (Freelist2_Node *)((uintptr_t)ptr + new_size);
    new_node->block_size = free_space;
    new_node->next = NULL;

    assert(freelist->used >= free_space);
    freelist->used -= free_space;

    Freelist2_Node *curr = freelist->head;
    Freelist2_Node *prev = NULL;

    while (curr && (uintptr_t)curr < (uintptr_t)new_node) {
        prev = curr;
        curr = curr->next;
    }

    freelist2_node_insert(freelist, prev, new_node);

    if (new_node->next && (uintptr_t)new_node + new_node->block_size == (uintptr_t)new_node->next) {
        new_node->block_size += new_node->next->block_size;
        freelist2_node_remove(freelist, new_node, new_node->next);
    }

    if (prev && (uintptr_t)prev + prev->block_size == (uintptr_t)new_node) {
        prev->block_size += new_node->block_size;
        freelist2_node_remove(freelist, prev, new_node);
    }

    return ptr;
}

void
freelist2_node_insert(Freelist2 *fl, Freelist2_Node *prev_node, Freelist2_Node *new_node)
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
freelist2_node_remove(Freelist2 *fl, Freelist2_Node *prev_node, Freelist2_Node *del_node)
{
    if (prev_node == NULL) {
        fl->head = del_node->next;
    } else {
        prev_node->next = del_node->next;
    }
    --fl->block_count;
    assert(fl->block_count >= 0);
}

void
freelist2_defragment(Freelist2 *fl)
{
    Freelist2_Node *currentFreeblock = fl->head;

    // continue while there are more freeblocks or we haven't reached the end of managed memory store.
    while (currentFreeblock->next != NULL ||
           ((uintptr_t)currentFreeblock + currentFreeblock->block_size) != ((uintptr_t)fl->data + fl->size))
    {
        size_t currentFreeSize = currentFreeblock->block_size;
        const uintptr_t nextAllocationStart = (uintptr_t)currentFreeblock + currentFreeSize;

        // the allocation pointing to memory that is present immediately after the current freeblock.
        void *allocationPtr = NULL;
        size_t headerSize = sizeof(Freelist2_Allocation_Header);

        // Check if there is an allocation immediately after the freeblock.
        htget(ptr_ptr, &fl->table, nextAllocationStart, (uintptr_t *)&allocationPtr);
        assert(allocationPtr != NULL);

        // Get the allocation header that immediately precedes the actual allocation.
        Freelist2_Allocation_Header *allocHeader = freelist2_get_header(allocationPtr);
        uintptr_t currentAllocEnd = nextAllocationStart + allocHeader->block_size;

        // get the padding that was required to satisfy the alignment_requirement for this allocation.
        const size_t currentPadding = (uintptr_t)allocHeader - (uintptr_t)nextAllocationStart;
        // verify that our padding calculation is indeed correct.
        assert(currentPadding == allocHeader->alignment_padding);

        // this is the actual data + header size without the alignment padding.
        size_t dataSizeWithoutPadding = allocHeader->block_size - currentPadding;

        // Calculate the new aligned position for this allocation after it is moved down.
        uintptr_t newAlignedAddress = align_forward((uintptr_t)currentFreeblock + headerSize,
                                                    allocHeader->alignment_requirement);
        assert(newAlignedAddress >= ((uintptr_t)currentFreeblock + headerSize));
        const size_t newPaddingSize = newAlignedAddress - ((uintptr_t)currentFreeblock + headerSize);

        // Update total used space based on padding difference.
        if (newPaddingSize > currentPadding) {
            fl->used += (newPaddingSize - currentPadding);
        } else {
            assert(fl->used >= (currentPadding - newPaddingSize));
            fl->used -= (currentPadding - newPaddingSize);
        }

        // Calculate new positions and update the header.
        uintptr_t newHeaderPosition = newAlignedAddress - headerSize;
        uintptr_t currentHeaderPosition = (uintptr_t)allocHeader;

        size_t newTotalSize = dataSizeWithoutPadding + newPaddingSize;
        allocHeader->block_size = newTotalSize;
        allocHeader->alignment_padding = newPaddingSize;
        *allocHeader->pOwner = (void *)newAlignedAddress;

        // making a copy of the current freenode since after the memmove, its memory will be corrupted.
        // this could be the free block node that the current node merges with.
        Freelist2_Node *currentFreenodeNext = currentFreeblock->next;

        // move the allocation down to its new position.
        memmove((void *)newHeaderPosition, (void *)currentHeaderPosition, dataSizeWithoutPadding);

        // update the hash table with the new position for this allocation(since it was moved).
        htdel(ptr_ptr, &fl->table, nextAllocationStart);
        htpush(ptr_ptr, &fl->table, (uintptr_t)currentFreeblock, newAlignedAddress);

        // Calculate the new freeblock size after moving allocation.
        uintptr_t newAllocEnd = (uintptr_t)currentFreeblock + newTotalSize;
        // verify that the allocation was moved down.
        assert(newAllocEnd <= currentAllocEnd);
        size_t newFreeSpace = currentAllocEnd - newAllocEnd;

        // Remove the current freeblock and create a new one.
        Freelist2_Node *movedCurrentFreeblock = (Freelist2_Node *)newAllocEnd;
        movedCurrentFreeblock->block_size = newFreeSpace;
        movedCurrentFreeblock->next = currentFreenodeNext;
        // Since this will always be the first block in the freelist, point the head to it.
        fl->head = movedCurrentFreeblock;

        // if the newlyMoved freeblock is adjacent to the next freeblock, merge both of them and set the pointers
        // correctly. the coalescene function handles the case where these two are not adjacent.
        freelist_merge_blocks_if_adjacent(fl, movedCurrentFreeblock, movedCurrentFreeblock->next);
        currentFreeblock = movedCurrentFreeblock;
    }

    // verify only one freenode left after defragmentation.
    assert(currentFreeblock == fl->head && currentFreeblock->next == NULL);
    // the freenode is pushed to the end of the freelist
    assert(((uintptr_t)currentFreeblock + currentFreeblock->block_size) == ((uintptr_t)fl->data + fl->size));

#if _DEBUG
    size_t freeSpace = freelist2_remaining_space(fl);
    assert(currentFreeblock->block_size == freeSpace);
    assert(currentFreeblock->block_size + fl->used == fl->size);
#endif
}

#ifdef FREELIST2_ALLOCATOR_UNIT_TESTS
static void
freelist2_test_initial_state(Freelist2 *fl)
{
    assert(fl->head == fl->data);
    assert(fl->block_count == 1);
    assert(fl->used == 0);
    assert(fl->head->block_size == fl->size);
}

static void
freelist2_verify_blocks(Freelist2 *fl, size_t *exp_free_sizes, int exp_block_count, bool print_block_size)
{
    int counter = 0;
    Freelist2_Node *curr = fl->head;
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
freelist2_validate_order(Freelist2 *fl)
{
    Freelist2_Node *current = fl->head;
    while (current && current->next)
    {
        // Verify addresses are in ascending order
        assert((uintptr_t)current < (uintptr_t)current->next);
        // Verify no overlapping blocks, and they should not be equal since that would be a bug(freenodes
        // side-by-side should have been merged)
        assert((uintptr_t)current + current->block_size < (uintptr_t)current->next);
        current = current->next;
    }
}

// Helper function to validate used memory accounting
static void
freelist2_validate_used_memory(Freelist2 *fl, void *base_addr, size_t total_size)
{
    size_t free_mem = 0;
    Freelist2_Node *current = fl->head;

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
freelist2_validate_memory(Freelist2 *fl, void *base_addr, size_t total_size)
{
    freelist2_validate_order(fl);
    freelist2_validate_used_memory(fl, base_addr, total_size);
}


void
freelist2_realloc_tests()
{
    printf("Running realloc tests ...\n");

    // Initialize with 1MB of memory
    const size_t mem_size = 1024 * 1024;
    void *memory = malloc(mem_size);
    assert(memory != NULL);

    Freelist2 fl;
    fl.policy = PLACEMENT_POLICY_FIND_BEST;
    freelist2_init(&fl, memory, mem_size, DEFAULT_ALIGNMENT);
    alloc_api *api = &fl.api;

    printf("1. Shrink down realloc test...\n");
    {
        size_t old_p1_size = 128;
        unsigned char *p1 = (unsigned char *)freelist2_alloc(&fl, old_p1_size);
        size_t u1 = fl.used;
        for (unsigned char i = 0; i < 128; ++i) {
            p1[i] = i;
        }
        unsigned char *p2 = (unsigned char *)freelist2_alloc(&fl, 256);
        size_t u2 = fl.used - u1;
        for (unsigned char i = 0; i < 255; ++i) {
            p2[i] = i;
        }
        unsigned char *p3 = (unsigned char *)freelist2_alloc(&fl, 32);
        size_t u3 = fl.used - (u1+u2);
        for (unsigned char i = 0; i < 32; ++i) {
            p3[i] = i;
        }
        freelist2_free(&fl, p2); p2 = NULL;
        uintptr_t old_head = (uintptr_t)fl.head;
        size_t new_p1_size = 120;
        unsigned char *new_p1 = (unsigned char *)freelist2_realloc(&fl, p1, new_p1_size);
        uintptr_t new_head = (uintptr_t)fl.head;
        size_t p1_shrink_size = old_p1_size - new_p1_size;
        assert(old_head > new_head && (new_head + p1_shrink_size) == old_head);
        for (int i = 0; i < new_p1_size; ++i) {
            assert(p1[i] == i);
        }
        assert((uintptr_t)p1 == (uintptr_t)new_p1);
        Freelist2_Node *first_free_node = fl.head;
        assert(first_free_node->block_size == u2+8);
        freelist2_free(&fl, p1);
        freelist2_free(&fl, p3);
        freelist2_test_initial_state(&fl);

        unsigned char *ptr1 = (unsigned char *)freelist2_alloc(&fl, 128);
        for (unsigned char i = 0; i < 128; ++i) {
            ptr1[i] = (i+1);
        }

        unsigned char *ptr2 = (unsigned char *)freelist2_alloc(&fl, 512);
        memset(ptr2, 0x0a, 512);

        ptr1 = (unsigned char *)freelist2_realloc(&fl, ptr1, 64);
        for (unsigned char i = 0; i < 64; ++i) {
            assert(ptr1[i] == (i+1));
        }

        assert(freelist2_remaining_space(&fl) + fl.used == fl.size);
        freelist2_free(&fl, ptr1);
        freelist2_free(&fl, ptr2);
        freelist2_test_initial_state(&fl);
    }

    printf("2. Basic realloc tests...\n");
    {
        // Test growing allocation
        void *ptr1 = freelist2_alloc(&fl, 128);
        assert(ptr1 != NULL);
        memset(ptr1, 0xAA, 128);

        size_t used_before = fl.used;
        void *ptr2 = freelist2_realloc(&fl, ptr1, 256);
        assert(ptr2 != NULL);
        // Verify content was preserved
        for (size_t i = 0; i < 128; i++) {
            assert(((unsigned char*)ptr2)[i] == 0xAA);
        }
        assert(fl.used >= used_before);
        freelist2_validate_memory(&fl, memory, mem_size);

        // Test shrinking allocation
        used_before = fl.used;
        ptr1 = freelist2_realloc(&fl, ptr2, 64);
        assert(ptr1 != NULL);
        // Verify content was preserved
        for (size_t i = 0; i < 64; i++) {
            assert(((unsigned char*)ptr1)[i] == 0xAA);
        }
        assert(fl.used <= used_before);
        freelist2_validate_memory(&fl, memory, mem_size);

        freelist2_free(&fl, ptr1);
        freelist2_test_initial_state(&fl);
    }

    printf("3. Edge case tests...\n");
    {
        // NULL ptr (should behave like alloc)
        void *ptr1 = freelist2_realloc(&fl, NULL, 128);
        assert(ptr1 != NULL);
        memset(ptr1, 0xBB, 128);
        freelist2_validate_memory(&fl, memory, mem_size);

        // size 0 (should behave like free)
        size_t used_before = fl.used;
        void *ptr2 = freelist2_realloc(&fl, ptr1, 0);
        assert(ptr2 == NULL);
        assert(fl.used < used_before);
        freelist2_validate_memory(&fl, memory, mem_size);

        ptr1 = freelist2_alloc(&fl, 256);
        assert(ptr1 != NULL);
        memset(ptr1, 0xCC, 256);
        used_before = fl.used;
        // with same size
        ptr2 = freelist2_realloc(&fl, ptr1, 256);
        assert(ptr2 != NULL);
        assert(fl.used == used_before);
        for (size_t i = 0; i < 256; i++) {
            assert(((unsigned char*)ptr2)[i] == 0xCC);
        }
        freelist2_validate_memory(&fl, memory, mem_size);
        freelist2_free(&fl, ptr2);

        freelist2_test_initial_state(&fl);
    }

    printf("4. Mixed allocation pattern tests...\n");
    {
        void *ptrs[10] = {NULL};
        size_t sizes[10] = {64, 128, 256, 512, 1024, 32, 96, 160, 384, 768};

        // Allocate with varying sizes
        for (int i = 0; i < 10; i++) {
            ptrs[i] = freelist2_alloc(&fl, sizes[i]);
            assert(ptrs[i] != NULL);
            memset(ptrs[i], i+1, sizes[i]);
            freelist2_validate_memory(&fl, memory, mem_size);
        }

        // Free some blocks to create holes
        freelist2_free(&fl, ptrs[1]); ptrs[1] = NULL;
        freelist2_free(&fl, ptrs[3]); ptrs[3] = NULL;
        freelist2_free(&fl, ptrs[5]); ptrs[5] = NULL;
        freelist2_free(&fl, ptrs[7]); ptrs[7] = NULL;
        freelist2_validate_memory(&fl, memory, mem_size);

        // Reallocate remaining blocks with various sizes
        size_t new_sizes[10] = {32, 0, 512, 0, 2048, 0, 128, 0, 256, 1536};
        for (int i = 0; i < 10; i++) {
            if (ptrs[i] != NULL) {
                void *new_ptr = freelist2_realloc(&fl, ptrs[i], new_sizes[i]);
                if (new_sizes[i] > 0) {
                    assert(new_ptr != NULL);
                    ptrs[i] = new_ptr;
                } else {
                    assert(new_ptr == NULL);
                    ptrs[i] = NULL;
                }
                freelist2_validate_memory(&fl, memory, mem_size);
            }
        }

        // Free remaining blocks
        for (int i = 0; i < 10; i++) {
            if (ptrs[i] != NULL) {
                freelist2_free(&fl, ptrs[i]);
                freelist2_validate_memory(&fl, memory, mem_size);
            }
        }

        freelist2_test_initial_state(&fl);
    }

    freelist2_test_initial_state(&fl);
    freelist2_free_all(&fl);
    freelist2_test_initial_state(&fl);
    free(memory);
    printf("All realloc tests passed successfully!\n");
}

void
freelist2_fragmentation_tests()
{
    // Setup 64MB memory pool with BEST FIT policy
    const size_t MEM_SIZE = 64 * 1024 * 1024; // 64 MB
    uint8_t *memory = malloc(MEM_SIZE);
    Freelist2 fl;
    freelist2_init(&fl, memory, MEM_SIZE, DEFAULT_ALIGNMENT);
    fl.policy = PLACEMENT_POLICY_FIND_BEST;

    // Test 1: Create many small allocations followed by selective frees
    void *ptrs[1000];
    size_t sizes[1000];
    size_t block_sizes[1000];

    // Allocate blocks of varying sizes
    for (int i = 0; i < 1000; i++) {
        sizes[i] = 1024 + (i % 64) * 512; // Varying sizes from 1KB to 32KB
        ptrs[i] = freelist2_alloc(&fl, sizes[i]);
        assert(ptrs[i] != NULL);
        block_sizes[i] = freelist2_block_size(ptrs[i]);
    }

    // Free every third block to create fragmentation
    for (int i = 0; i < 1000; i += 3) {
        freelist2_free(&fl, ptrs[i]);
    }

    // Test coalescing by tracking exact block sizes
    for (int i = 1; i < 999; i += 3) {
        size_t before_free = freelist2_remaining_space(&fl);
        size_t block_i = freelist2_block_size(ptrs[i]);
        freelist2_free(&fl, ptrs[i]);
        assert(freelist2_remaining_space(&fl) == before_free + block_i);

        size_t block_i_plus_1 = freelist2_block_size(ptrs[i + 1]);
        freelist2_free(&fl, ptrs[i + 1]);
        assert(freelist2_remaining_space(&fl) == before_free + block_i + block_i_plus_1);
    }

    // Test 2: Test coalescing edge cases
    void *edge_ptrs[5];
    size_t edge_block_sizes[5];
    size_t block_size = 1024;

    // Allocate 5 consecutive blocks
    for (int i = 0; i < 5; i++) {
        edge_ptrs[i] = freelist2_alloc(&fl, block_size);
        assert(edge_ptrs[i] != NULL);
        edge_block_sizes[i] = freelist2_block_size(edge_ptrs[i]);
    }

    // Test coalescing patterns:
    // 1. Free middle block
    freelist2_free(&fl, edge_ptrs[2]);
    size_t mid_free = freelist2_remaining_space(&fl);

    // 2. Free left adjacent block - should coalesce
    size_t ptr_1_block_size = freelist2_block_size(edge_ptrs[1]);
    freelist2_free(&fl, edge_ptrs[1]);
    assert(freelist2_remaining_space(&fl) == (mid_free + ptr_1_block_size));

    // 3. Free right adjacent block - should coalesce
    size_t ptr_3_block_size = freelist2_block_size(edge_ptrs[3]);
    freelist2_free(&fl, edge_ptrs[3]);
    assert(freelist2_remaining_space(&fl) == (mid_free + ptr_1_block_size + ptr_3_block_size));

    // Free remaining edge pointers
    freelist2_free(&fl, edge_ptrs[0]);
    freelist2_free(&fl, edge_ptrs[4]);

    // Test 3: Stress realloc with accurate block size tracking
    void *realloc_ptr = freelist2_alloc(&fl, 1024);
    assert(realloc_ptr != NULL);
    void *last_ptr = realloc_ptr;

    // Repeatedly grow and shrink, tracking actual block sizes
    for (int i = 0; i < 100; i++) {
        size_t current_block_size = freelist2_block_size(realloc_ptr);
        size_t before_realloc = freelist2_remaining_space(&fl);
        size_t new_size = 1024 * (1 + (i % 10));

        void *new_ptr = freelist2_realloc(&fl, realloc_ptr, new_size);
        assert(new_ptr != NULL);
        last_ptr = new_ptr;

        size_t new_block_size = freelist2_block_size(new_ptr);
        realloc_ptr = new_ptr;
    }

    freelist2_free(&fl, last_ptr);

    // Test 4: Edge case - maximum size allocation
    void *max_ptr = freelist2_alloc(&fl, MEM_SIZE - sizeof(Freelist2_Allocation_Header));
    assert(max_ptr != NULL);                    // Should succeed as all memory is free and coalesced
    assert(freelist2_remaining_space(&fl) == 0); // All memory should be allocated

    // Free max_ptr
    freelist2_free(&fl, max_ptr);
    assert(freelist2_remaining_space(&fl) == MEM_SIZE); // Should return to full free space

    // --------------------------------------------------------------------------------------------
    // Chaotic allocation test with prime sizes
    void *chaos_ptrs[500];
    size_t prime_sizes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};
    size_t num_primes = sizeof(prime_sizes) / sizeof(prime_sizes[0]);

    // Random allocations with prime-based sizes
    for (int i = 0; i < 500; i++) {
        size_t base = prime_sizes[i % num_primes];
        size_t size = base * 1023 + (i * 17) % 2048; // Weird sizes
        chaos_ptrs[i] = freelist2_alloc(&fl, size);
        assert(chaos_ptrs[i] != NULL);
    }

    // Free in strange pattern: every prime index
    for (int i = 0; i < 500; i++) {
        for (int j = 0; j < num_primes; j++) {
            if (i == prime_sizes[j]) {
                freelist2_free(&fl, chaos_ptrs[i]);
                chaos_ptrs[i] = NULL;
                break;
            }
        }
    }

    // Realloc every 7th remaining pointer to a size based on current free space
    for (int i = 0; i < 500; i += 7) {
        if (chaos_ptrs[i] != NULL) {
            size_t current_free = freelist2_remaining_space(&fl);
            size_t new_size = (current_free % 4096)+1024; // Random but bounded size
            void *new_ptr = freelist2_realloc(&fl, chaos_ptrs[i], new_size);
            assert(new_ptr != NULL);
            chaos_ptrs[i] = new_ptr;
        }
    }

    // Allocate tiny blocks between existing allocations
    void *tiny_ptrs[200];
    for (int i = 0; i < 200; i++) {
        tiny_ptrs[i] = freelist2_alloc(&fl, (i%5)*8+16); // Tiny sizes: 16, 24, 32, 40, 48 bytes
    }

    // Free alternating blocks: odd indices from chaos_ptrs, even indices from tiny_ptrs
    for (int i = 1; i < 500; i += 2) {
        if (chaos_ptrs[i] != NULL) {
            freelist2_free(&fl, chaos_ptrs[i]);
            chaos_ptrs[i] = NULL;
        }
    }

    for (int i = 0; i < 200; i += 2) {
        freelist2_free(&fl, tiny_ptrs[i]);
        tiny_ptrs[i] = NULL;
    }

    // Try medium allocations to fill gaps
    void *medium_ptrs[50];
    for (int i = 0; i < 50; i++) {
        size_t size = 512 * (i % 7 + 1); // Sizes from 512 to 3584 in 512-byte steps
        medium_ptrs[i] = freelist2_alloc(&fl, size);
    }

    // Free remaining blocks in reverse order
    for (int i = 199; i >= 0; i--) {
        if (i % 2 == 1)
        { // Free odd indices of tiny_ptrs
            freelist2_free(&fl, tiny_ptrs[i]);
            tiny_ptrs[i] = NULL;
        }
    }

    for (int i = 499; i >= 0; i--) {
        if (chaos_ptrs[i] != NULL) {
            freelist2_free(&fl, chaos_ptrs[i]);
            chaos_ptrs[i] = NULL;
        }
    }

    for (int i = 49; i >= 0; i--) {
        if (medium_ptrs[i] != NULL) {
            freelist2_free(&fl, medium_ptrs[i]);
            medium_ptrs[i] = NULL;
        }
    }

    // Verify we can still allocate the maximum block
    max_ptr = freelist2_alloc(&fl, MEM_SIZE - sizeof(Freelist2_Allocation_Header));
    assert(max_ptr != NULL);
    freelist2_free(&fl, max_ptr);

    // Verify all memory is recovered
    assert(freelist2_remaining_space(&fl) == MEM_SIZE);
    // --------------------------------------------------------------------------------------------

    // Cleanup
    free(memory);
    printf("fragmentation tests passed successfully!\n\n");
}

void
freelist2_alignment_tests()
{
    printf("freelist alignment tests!\n");

    // Initialize with 64MB of memory
    const size_t mem_size = 64 * 1024 * 1024;
    void *memory = malloc(mem_size);
    assert(memory != NULL);

    Freelist2 fl;
    fl.policy = PLACEMENT_POLICY_FIND_BEST;
    freelist2_init(&fl, memory, mem_size, DEFAULT_ALIGNMENT);

    // Verify initial state
    assert(fl.head != NULL);
    assert((uintptr_t)fl.head >= (uintptr_t)memory);
    assert(fl.used == 0);
    freelist2_validate_order(&fl);
    freelist2_validate_used_memory(&fl, memory, mem_size);

    const size_t alloc_header_size = sizeof(Freelist2_Allocation_Header);

    printf("1. Basic allocation tests with different alignments...\n");
    {
        // exhaust all the memory available
        size_t size = mem_size;
        void *ptr1 = freelist2_alloc(&fl, size);
        // Not enough space for allocation header
        assert(ptr1 == NULL);
        size = mem_size - (sizeof(Freelist2_Allocation_Header));
        ptr1 = freelist2_alloc(&fl, size);
        // Allocates entire memory as header fits
        assert((ptr1 != NULL) && (fl.head == NULL) &&
               (fl.block_count == 0) && (fl.used == mem_size));
        freelist2_free(&fl, ptr1);
        size_t remaining = freelist2_remaining_space(&fl);
        assert(remaining == mem_size);
        size = mem_size - sizeof(Freelist2_Allocation_Header) - 1;
        ptr1 = freelist2_alloc(&fl, size);
        // 1 byte free but too small for headers, returns all memory
        assert((ptr1 != NULL) && (fl.head == NULL) &&
               (fl.block_count == 0) && (fl.used == mem_size));
        remaining = freelist2_remaining_space(&fl);
        assert(remaining == 0);
        freelist2_free(&fl, ptr1);
        remaining = freelist2_remaining_space(&fl);
        assert(remaining == mem_size);
        size = mem_size - (2 * sizeof(Freelist2_Allocation_Header) + 1);
        ptr1 = freelist2_alloc(&fl, size);
        // Space left for free node, but only 1 usable byte due to header size
        remaining = freelist2_remaining_space(&fl);
        assert((ptr1 != NULL) && (fl.head != NULL) && (fl.block_count == 1) &&
               (fl.used == size + sizeof(Freelist2_Allocation_Header)));
        assert(remaining == sizeof(Freelist2_Allocation_Header)+1);
        // NOTE: Realloc: Grows allocation by 1 byte (should succeed since we have 1 byte allocatable)
        // the edgecase here: when we ask for 1 more byte, the remaining size in the freelist will equal the size
        // of the allocation header. In other words, that freespace is useless for any new allocation, since for
        // each allocation, the freelist needs atleast the header_size bytes, so what the freelist does is allocate
        // the extra sizeof(header) bytes to the asked allocation size and returns the memory. In this case, there
        // should be 0 remaining usable size in the freelist.
        ptr1 = freelist2_realloc(&fl, ptr1, size+1);
        remaining = freelist2_remaining_space(&fl);
        assert((ptr1 != NULL) && (fl.head == NULL) &&
               (fl.block_count == 0) && (fl.used == fl.size));
        // Realloc: Shrinks allocation back to original size. The remaining space will be sizeof(header)+1 byte
        ptr1 = freelist2_realloc(&fl, ptr1, size);
        remaining = freelist2_remaining_space(&fl);
        assert((ptr1 != NULL) && (fl.head != NULL) && (fl.block_count == 1) &&
               (fl.used == (mem_size - (sizeof(Freelist2_Allocation_Header) + 1))) &&
               (fl.head->block_size == (sizeof(Freelist2_Allocation_Header) + 1)));
        freelist2_free(&fl, ptr1);
        remaining = freelist2_remaining_space(&fl);
        assert(remaining == mem_size);
        void *ptr = freelist2_alloc(&fl, 32);
        void *new_ptr = freelist2_realloc(&fl, ptr, (mem_size+1));
        assert(new_ptr == NULL); // Should fail as size exceeds pool
        freelist2_free_all(&fl);

        // Test allocations with various alignments
        const size_t alignments[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};
        void *ptrs[sizeof(alignments)/sizeof(alignments[0])] = {NULL};

        size_t total_allocated = 0;
        size_t expected_allocated = 0;
        // Pre-calculate expected total allocation
        for (int i = 0; i < sizeof(alignments)/sizeof(alignments[0]); ++i) {
            expected_allocated += 128; // alloc_size is fixed at 128 which is > sizeof(Freelist2_Allocation_Header)
        }

        for (size_t i = 0; i < sizeof(alignments)/sizeof(alignments[0]); i++)
        {
            size_t alignment = alignments[i];
            size_t alloc_size = 128;

            size_t used_before = fl.used;
            ptrs[i] = freelist2_alloc_align(&fl, alloc_size, alignment);
            assert(ptrs[i] != NULL);
            assert((uintptr_t)ptrs[i] % alignment == 0);

            Freelist2_Allocation_Header *header = (Freelist2_Allocation_Header *)((char *)ptrs[i] - alloc_header_size);
            assert((fl.used - used_before) == (header->block_size));
            size_t actual_alloc = (header->block_size - header->alignment_padding - alloc_header_size);
            assert(actual_alloc == alloc_size);
            total_allocated += actual_alloc;

            memset(ptrs[i], 0xAA, alloc_size);
            freelist2_validate_order(&fl);
            freelist2_validate_used_memory(&fl, memory, mem_size);

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
                Freelist2_Allocation_Header *header =
                    (Freelist2_Allocation_Header *)((char *)ptrs[i] - alloc_header_size);
                size_t block_size = header->block_size;

                freelist2_free(&fl, ptrs[i]);
                assert(fl.used == used_before - block_size);
                freelist2_validate_order(&fl);
                freelist2_validate_used_memory(&fl, memory, mem_size);
            }
        }
    }


    freelist2_free_all(&fl);
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

        void *allocations[sizeof(test_cases)/sizeof(test_cases[0])] = {NULL};
        Freelist2_Allocation_Header allocationHeaders[sizeof(test_cases)/sizeof(test_cases[0])] = {};

        size_t total_allocated_minus_padding = 0;
        size_t expected_allocated = 0;
        for (int i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); ++i) {
            expected_allocated += test_cases[i].size > sizeof(Freelist2_Node) ? test_cases[i].size
                                                                             : sizeof(Freelist2_Node);
        }

        for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
            size_t alloc_size = test_cases[i].size;
            size_t alignment = test_cases[i].alignment;

            size_t used_before = fl.used;
            allocations[i] = freelist2_alloc_align(&fl, alloc_size, alignment);
            assert(allocations[i] != NULL);
            assert((uintptr_t)allocations[i] % alignment == 0);

            Freelist2_Allocation_Header *header = (Freelist2_Allocation_Header *)((char *)allocations[i] - alloc_header_size);
            allocationHeaders[i] = *header;
            assert((fl.used - used_before) == (header->block_size));
            size_t actual_alloc = (header->block_size - header->alignment_padding - alloc_header_size);
            assert(actual_alloc == alloc_size || actual_alloc == sizeof(Freelist2_Node));
            total_allocated_minus_padding += actual_alloc;

            // Fill allocated memory with pattern
            memset(allocations[i], 0xBB + i, alloc_size);

            freelist2_validate_order(&fl);
            freelist2_validate_used_memory(&fl, memory, mem_size);
        }
        assert(total_allocated_minus_padding == expected_allocated);
        assert(fl.block_count == 1);
        // Free in mixed order to test coalescing
        size_t free_order[] = {0, 2, 4, 1, 6, 3, 5, 7};
        int exp_block_counts[] = {2, 3, 4, 3, 4, 3, 2, 1};
        size_t exp_free_blocks[8][4] = {
                                            {96,   0,    0,   0}, // first fb (96)
                                            {96,   1152, 0,   0}, // first fb (96), second fb(1152 bytes)
                                            {96,   1152, 48,  0}, // first fb (96), second fb(1152 bytes), third fb(48)
                                            {1552, 48,   0,   0},
                                            {1552, 48,   216, 0},
                                            {8128, 216,  0,   0},
                                            {9424, 0,    0,   0},
                                            {0,    0,    0,   0},
                                       };
        for (size_t i = 0; i < sizeof(free_order)/sizeof(free_order[0]); i++)
        {
            size_t allocatedBlockIndex = free_order[i];
            size_t size = test_cases[allocatedBlockIndex].size;
            size_t size_exp = size < sizeof(Freelist2_Node) ? sizeof(Freelist2_Node) : size;
            size_t alignment = test_cases[allocatedBlockIndex].alignment;

            Freelist2_Allocation_Header *header = (Freelist2_Allocation_Header *)((char *)allocations[allocatedBlockIndex] -
                                                                                sizeof(Freelist2_Allocation_Header));
            size_t allocation_block_size = header->block_size;
            // Calculate the total size that should be freed
            total_allocated_minus_padding -= (header->block_size -
                                              (header->alignment_padding + sizeof(Freelist2_Allocation_Header)));
            assert((header->block_size - header->alignment_padding - sizeof(Freelist2_Allocation_Header)) == size_exp);

            size_t used_before = fl.used;
            freelist2_free(&fl, allocations[allocatedBlockIndex]);
            assert(fl.used == used_before - allocation_block_size);

            assert(exp_block_counts[i] == fl.block_count);
            freelist2_verify_blocks(&fl, exp_free_blocks[i], exp_block_counts[i], false);

            freelist2_validate_order(&fl);
            freelist2_validate_used_memory(&fl, memory, mem_size);
        }
        assert((total_allocated_minus_padding == 0));
    }

    assert((fl.block_count == 1) && (fl.used == 0) && (fl.head->block_size == mem_size));
    freelist2_free_all(&fl);
    printf("3. Alignment stress tests...\n");
    {
        // Test worst-case alignment scenarios
        const size_t max_alignment = 4096;
        size_t used_before = fl.used;

        void *ptr1 = freelist2_alloc_align(&fl, 1, max_alignment);
        assert(ptr1 != NULL);
        assert((uintptr_t)ptr1 % max_alignment == 0);
        Freelist2_Allocation_Header *header1 = (Freelist2_Allocation_Header *)((char *)ptr1 - alloc_header_size);
        assert((fl.used - used_before) == header1->block_size);
        size_t actual_alloc1 = (header1->block_size - header1->alignment_padding - alloc_header_size);
        assert(actual_alloc1 == sizeof(Freelist2_Node)); // Minimum allocation size

        void *ptr2 = freelist2_alloc_align(&fl, 1, 8);
        assert(ptr2 != NULL);
        assert((uintptr_t)ptr2 % 8 == 0);
        Freelist2_Allocation_Header *header2 = (Freelist2_Allocation_Header *)((char *)ptr2 - alloc_header_size);
        assert((fl.used - (used_before + header1->block_size)) == header2->block_size);
        size_t actual_alloc2 = (header2->block_size - header2->alignment_padding - alloc_header_size);
        assert(actual_alloc2 == sizeof(Freelist2_Node)); // Minimum allocation size

        void *ptr3 = freelist2_alloc_align(&fl, 1, max_alignment);
        assert(ptr3 != NULL);
        assert((uintptr_t)ptr3 % max_alignment == 0);
        Freelist2_Allocation_Header *header3 = (Freelist2_Allocation_Header *)((char *)ptr3 - alloc_header_size);
        size_t actual_alloc3 = (header3->block_size - header3->alignment_padding - alloc_header_size);
        assert(actual_alloc3 == sizeof(Freelist2_Node)); // Minimum allocation size

        // Verify no overlap
        if (ptr1 < ptr2) {
            assert((uintptr_t)ptr1 + sizeof(Freelist2_Node) <= (uintptr_t)ptr2);
        } else {
            assert((uintptr_t)ptr2 + sizeof(Freelist2_Node) <= (uintptr_t)ptr1);
        }
        if (ptr2 < ptr3) {
            assert((uintptr_t)ptr2 + sizeof(Freelist2_Node) <= (uintptr_t)ptr3);
        } else {
            assert((uintptr_t)ptr3 + sizeof(Freelist2_Node) <= (uintptr_t)ptr2);
        }

        freelist2_free(&fl, ptr1);
        freelist2_free(&fl, ptr2);
        freelist2_free(&fl, ptr3);

        assert(fl.used == used_before);
        freelist2_validate_order(&fl);
        freelist2_validate_used_memory(&fl, memory, mem_size);
    }

    assert((fl.block_count == 1) && (fl.used == 0) && (fl.head->block_size == mem_size));
    free(memory);
    printf("All alignment tests passed successfully!\n");
}

// -------------------------------------------------------------------------------------------------------------------
// Test Automatic Memory Management
// -------------------------------------------------------------------------------------------------------------------
#include <time.h>

#define MEMORY_SIZE (2 * 1024 * 1024)
#define TINY_ALLOCATION 8
#define SMALL_ALLOCATION 32
#define MEDIUM_ALLOCATION 256
#define LARGE_ALLOCATION 1024
#define HUGE_ALLOCATION (4 * 1024)
#define MAX_ALLOCATIONS 2000

typedef struct TestAllocation
{
    void *ptr;
    size_t size;
    size_t alignment;
    unsigned char pattern;
    bool is_active;
} TestAllocation;

// Helper function to verify memory integrity
void freelist2_verify_memory_contents(void* ptr, size_t size, unsigned char pattern) {
    unsigned char* bytes = (unsigned char*)ptr;
    for (size_t i = 0; i < size; i++) {
        assert(bytes[i] == pattern);
    }
}

// Helper function to fill memory with a pattern
void freelist2_fill_memory_pattern(void* ptr, size_t size, unsigned char pattern) {
    memset(ptr, pattern, size);
}

// Test 1: Alternating allocations and frees
void freelist2_test_alternating_patterns() {
    Freelist2 fl = {};
    freelist2_init(&fl, malloc(MEMORY_SIZE), MEMORY_SIZE, DEFAULT_ALIGNMENT);

    TestAllocation allocations[100];
    int pattern = 0;

    printf("Test 1: Alternating allocations and frees...\n");

    // Create a checker-board pattern of allocations
    for (int i = 0; i < 100; i++) {
        allocations[i].ptr = freelist2_alloc(&fl, SMALL_ALLOCATION);
        freelist2_set_allocation_owner(&fl, allocations[i].ptr, &allocations[i].ptr);

        freelist2_fill_memory_pattern(allocations[i].ptr, SMALL_ALLOCATION, i);
        allocations[i].size = SMALL_ALLOCATION;
        allocations[i].is_active = true;
    }

    // Free every other allocation
    for (int i = 0; i < 100; i += 2) {
        if (allocations[i].is_active) {
            freelist2_verify_memory_contents(allocations[i].ptr, SMALL_ALLOCATION, i);
            freelist2_free(&fl, allocations[i].ptr);
            allocations[i].is_active = false;
        }
    }

    freelist2_defragment(&fl);

    // Verify remaining allocations are intact
    for (int i = 1; i < 100; i += 2) {
        if (allocations[i].is_active) {
            freelist2_verify_memory_contents(allocations[i].ptr, SMALL_ALLOCATION, i);
        }
    }

    free(fl.data);
}

// Test 2: Random allocation sizes
void freelist2_test_random_sizes() {
    Freelist2 fl = {};
    freelist2_init(&fl, malloc(MEMORY_SIZE), MEMORY_SIZE, DEFAULT_ALIGNMENT);

    printf("Test 2: Random allocation sizes...\n");

    TestAllocation allocations[MAX_ALLOCATIONS];
    int active_allocations = 0;

    srand(time(NULL));

    // Make random sized allocations
    for (int i = 0; i < MAX_ALLOCATIONS; i++) {
        size_t size = rand() % 1024 + 16;  // Random size between 16 and 1040 bytes

        allocations[i].ptr = freelist2_alloc(&fl, size);
        freelist2_set_allocation_owner(&fl, allocations[i].ptr, &allocations[i].ptr);
        if (allocations[i].ptr != NULL)
        {
            allocations[i].size = size;
            allocations[i].is_active = true;
            freelist2_fill_memory_pattern(allocations[i].ptr, size, i % 256);
        }
    }

    // Randomly free 50% of allocations
    for (int i = 0; i < MAX_ALLOCATIONS; i++) {
        if (rand() % 2 == 0 && allocations[i].is_active) {
            freelist2_verify_memory_contents(allocations[i].ptr, allocations[i].size, i % 256);
            freelist2_free(&fl, allocations[i].ptr);
            allocations[i].is_active = false;
        }
    }

    freelist2_defragment(&fl);

    // Verify remaining allocations
    for (int i = 0; i < MAX_ALLOCATIONS; i++) {
        if (allocations[i].is_active) {
            freelist2_verify_memory_contents(allocations[i].ptr, allocations[i].size, i % 256);
        }
    }

    free(fl.data);
}

// Test 3: Edge case - Maximum fragmentation
void freelist2_test_maximum_fragmentation() {
    Freelist2 fl = {};
    freelist2_init(&fl, malloc(MEMORY_SIZE), MEMORY_SIZE, DEFAULT_ALIGNMENT);

    printf("Test 3: Maximum fragmentation...\n");

    TestAllocation allocations[MAX_ALLOCATIONS];
    int count = 0;

    // Allocate alternating tiny and large blocks
    for (int i = 0; i < 1000 && count < 1000; i++) {
        size_t size = (i % 2 == 0) ? 16 : 256;

        allocations[count].ptr = freelist2_alloc(&fl, size);
        freelist2_set_allocation_owner(&fl, allocations[count].ptr, &allocations[count].ptr);

        if (allocations[count].ptr != NULL) {
            allocations[count].size = size;
            allocations[count].is_active = true;
            freelist2_fill_memory_pattern(allocations[count].ptr, size, i % 256);
            count++;
        } else {
            break;
        }
    }

    // Free all small blocks
    for (int i = 0; i < count; i++) {
        if (allocations[i].size == 16) {
            freelist2_verify_memory_contents(allocations[i].ptr, allocations[i].size, i % 256);
            freelist2_free(&fl, allocations[i].ptr);
            allocations[i].is_active = false;
        }
    }

    freelist2_defragment(&fl);

    // Verify large blocks are intact
    for (int i = 0; i < count; i++) {
        if (allocations[i].is_active) {
            freelist2_verify_memory_contents(allocations[i].ptr, allocations[i].size, i % 256);
        }
    }

    free(fl.data);
}

// Test 4: Alignment stress test
void freelist2_test_alignments() {
    Freelist2 fl = {};
    freelist2_init(&fl, malloc(MEMORY_SIZE), MEMORY_SIZE, DEFAULT_ALIGNMENT);

    printf("Test 4: Testing different alignments...\n");

    TestAllocation allocations[100];
    int count = 0;

    // Allocate with different alignments
    size_t alignments[] = {8, 16, 32, 64, 128};

    for (int i = 0; i < 100 && count < 100; i++) {
        size_t alignment = alignments[i % 5];
        size_t size = 32;

        allocations[count].ptr = freelist2_alloc_align(&fl, size, alignment);
        freelist2_set_allocation_owner(&fl, allocations[count].ptr, &allocations[count].ptr);
        if (allocations[count].ptr != NULL) {
            // Verify alignment
            assert(((uintptr_t)allocations[count].ptr % alignment) == 0);

            allocations[count].size = size;
            allocations[count].is_active = true;
            freelist2_fill_memory_pattern(allocations[count].ptr, size, i % 256);
            count++;
        }
    }

    // Free random allocations
    for (int i = 0; i < count; i += 2) {
        freelist2_verify_memory_contents(allocations[i].ptr, allocations[i].size, i % 256);
        freelist2_free(&fl, allocations[i].ptr);
        allocations[i].is_active = false;
    }

    freelist2_defragment(&fl);

    // Verify remaining alignments and contents
    for (int i = 0; i < count; i++) {
        if (allocations[i].is_active) {
            size_t alignment = alignments[i % 5];
            assert(((uintptr_t)allocations[i].ptr % alignment) == 0);
            freelist2_verify_memory_contents(allocations[i].ptr, allocations[i].size, i % 256);
        }
    }

    free(fl.data);
}

// Test 5: Fibonacci-based allocation pattern
void freelist2_test_fibonacci_pattern() {
    printf("Test 5: Fibonacci pattern allocations...\n");
    Freelist2 fl = {};
    freelist2_init(&fl, malloc(MEMORY_SIZE), MEMORY_SIZE, DEFAULT_ALIGNMENT);

    TestAllocation allocations[100];
    int count = 0;

    // Generate Fibonacci sequence for allocation sizes
    size_t fib1 = 16, fib2 = 32;
    for (int i = 0; i < 20 && count < 100; i++) {
        size_t size = fib1;

        // Allocate using address from TestAllocation struct
        allocations[count].ptr = freelist2_alloc(&fl, size);
        freelist2_set_allocation_owner(&fl, allocations[count].ptr, &allocations[count].ptr);
        if (allocations[count].ptr != NULL) {
            allocations[count].size = size;
            allocations[count].pattern = i;
            allocations[count].is_active = true;
            freelist2_fill_memory_pattern(allocations[count].ptr, size, i);
            count++;
        }

        size_t next = fib1 + fib2;
        fib1 = fib2;
        fib2 = next;
    }

    // Free Fibonacci numbers at odd indices
    for (int i = 1; i < count; i += 2) {
        freelist2_verify_memory_contents(allocations[i].ptr, allocations[i].size, allocations[i].pattern);
        freelist2_free(&fl, allocations[i].ptr);
        allocations[i].is_active = false;
    }

    freelist2_defragment(&fl);

    free(fl.data);
}

// Test 6: Prime number sized allocations
void freelist2_test_prime_sizes() {
    printf("Test 6: Prime-sized allocations...\n");
    Freelist2 fl = {};
    freelist2_init(&fl, malloc(MEMORY_SIZE), MEMORY_SIZE, DEFAULT_ALIGNMENT);

    TestAllocation allocations[100];
    size_t prime_sizes[] = {17, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71};
    int count = 0;

    // Allocate prime-sized blocks
    for (int i = 0; i < sizeof(prime_sizes) / sizeof(prime_sizes[0]) && count < 100; i++)
    {
        allocations[count].ptr = freelist2_alloc(&fl, prime_sizes[i]);
        freelist2_set_allocation_owner(&fl, allocations[count].ptr, &allocations[count].ptr);
        if (allocations[count].ptr != NULL) {
            allocations[count].size = prime_sizes[i];
            allocations[count].pattern = i;
            allocations[count].is_active = true;
            freelist2_fill_memory_pattern(allocations[count].ptr, prime_sizes[i], i);
            count++;
        }
    }

    // Free blocks with prime index
    for (int i = 2; i < count; i++) {
        bool is_prime = true;
        for (int j = 2; j * j <= i; j++) {
            if (i % j == 0) {
                is_prime = false;
                break;
            }
        }
        if (is_prime) {
            freelist2_verify_memory_contents(allocations[i].ptr, allocations[i].size, allocations[i].pattern);
            freelist2_free(&fl, allocations[i].ptr);
            allocations[i].is_active = false;
        }
    }

    freelist2_defragment(&fl);

    free(fl.data);
}

// Test 7: Pyramid pattern
void freelist2_test_pyramid_pattern() {
    printf("Test 7: Pyramid pattern allocations...\n");
    Freelist2 fl = {};
    freelist2_init(&fl, malloc(MEMORY_SIZE), MEMORY_SIZE, DEFAULT_ALIGNMENT);

    TestAllocation allocations[MAX_ALLOCATIONS];
    int count = 0;

    // Create pyramid pattern: increasing then decreasing sizes
    size_t current_size = 16;
    bool increasing = true;

    while (count < MAX_ALLOCATIONS && current_size > 0) {

        allocations[count].ptr = freelist2_alloc(&fl, current_size);
        freelist2_set_allocation_owner(&fl, allocations[count].ptr, &allocations[count].ptr);
        if (allocations[count].ptr != NULL) {
            allocations[count].size = current_size;
            allocations[count].pattern = count % 256;
            allocations[count].is_active = true;
            freelist2_fill_memory_pattern(allocations[count].ptr, current_size, count % 256);
            count++;
        }

        if (increasing) {
            current_size *= 2;
            if (current_size > 1024) {
                increasing = false;
                current_size = 512;
            }
        } else {
            current_size /= 2;
        }
    }

    // Free middle portion of pyramid
    size_t quarter = count / 4;
    for (size_t i = quarter; i < 3 * quarter; i++) {
        freelist2_verify_memory_contents(allocations[i].ptr, allocations[i].size, allocations[i].pattern);
        freelist2_free(&fl, allocations[i].ptr);
        allocations[i].is_active = false;
    }

    freelist2_defragment(&fl);

    free(fl.data);
}

// Test 8: Extreme alignment requirements
void freelist2_test_extreme_alignments() {
    printf("Test 8: Extreme alignment requirements...\n");
    Freelist2 fl = {};
    freelist2_init(&fl, malloc(MEMORY_SIZE), MEMORY_SIZE, DEFAULT_ALIGNMENT);

    TestAllocation allocations[100];
    int count = 0;

    // Array of power-of-two alignments
    size_t alignments[] = {16, 32, 64, 128, 256, 512, 1024};

    // Allocate with increasing alignment requirements
    for (int i = 0; i < sizeof(alignments)/sizeof(alignments[0]) && count < 100; i++) {

        allocations[count].ptr = freelist2_alloc_align(&fl, SMALL_ALLOCATION, alignments[i]);
        freelist2_set_allocation_owner(&fl, allocations[count].ptr, &allocations[count].ptr);
        if (allocations[count].ptr != NULL) {
            allocations[count].size = SMALL_ALLOCATION;
            allocations[count].alignment = alignments[i];
            allocations[count].pattern = i;
            allocations[count].is_active = true;

            // Verify alignment
            assert(((uintptr_t)allocations[count].ptr % alignments[i]) == 0);
            freelist2_fill_memory_pattern(allocations[count].ptr, SMALL_ALLOCATION, i);
            count++;
        }
    }

    freelist2_defragment(&fl);

    // Verify alignments are maintained
    for (int i = 0; i < count; i++) {
        if (allocations[i].is_active) {
            assert(((uintptr_t)allocations[i].ptr % allocations[i].alignment) == 0);
            freelist2_verify_memory_contents(allocations[i].ptr, allocations[i].size, allocations[i].pattern);
        }
    }

    free(fl.data);
}

// Test 9: Interlaced allocations
void freelist2_test_interlaced_pattern() {
    printf("Test 9: Interlaced allocation pattern...\n");
    Freelist2 fl = {};
    freelist2_init(&fl, malloc(MEMORY_SIZE), MEMORY_SIZE, DEFAULT_ALIGNMENT);

    TestAllocation allocations[MAX_ALLOCATIONS];
    int count = 0;

    // Create three different size categories
    size_t sizes[] = {TINY_ALLOCATION, MEDIUM_ALLOCATION, HUGE_ALLOCATION};

    // Allocate in an interlaced pattern
    for (int i = 0; i < 100 && count < MAX_ALLOCATIONS; i++) {
        for (int j = 0; j < 3 && count < MAX_ALLOCATIONS; j++) {

            allocations[count].ptr = freelist2_alloc(&fl, sizes[j]);
            freelist2_set_allocation_owner(&fl, allocations[count].ptr, &allocations[count].ptr);
            if (allocations[count].ptr != NULL) {
                allocations[count].size = sizes[j];
                allocations[count].pattern = count % 256;
                allocations[count].is_active = true;
                freelist2_fill_memory_pattern(allocations[count].ptr, sizes[j], count % 256);
                count++;
            }
        }
    }

    // Free every third allocation
    for (int i = 0; i < count; i += 3) {
        freelist2_verify_memory_contents(allocations[i].ptr, allocations[i].size, allocations[i].pattern);
        freelist2_free(&fl, allocations[i].ptr);
        allocations[i].is_active = false;
    }

    freelist2_defragment(&fl);

    free(fl.data);
}

void
freelist2_test_fragmentation_recovery()
{
    printf("Test 10: Testing fragmentation recovery...\n");
    Freelist2 fl = {};
    freelist2_init(&fl, malloc(MEMORY_SIZE), MEMORY_SIZE, DEFAULT_ALIGNMENT);

    TestAllocation allocations[MAX_ALLOCATIONS];
    int count = 0;
    size_t total_allocated = 0;

    size_t allocation_unit = sizeof(Freelist2_Node);  // 16 bytes minimum
    size_t overhead_per_alloc = sizeof(Freelist2_Allocation_Header);
    size_t total_size_per_alloc = allocation_unit + overhead_per_alloc;

    printf("\tMemory stats:\n");
    printf("\t- Total memory: %zu bytes\n", (size_t)MEMORY_SIZE);
    printf("\t- Size per allocation: %zu bytes (minimum: %zu, header: %zu)\n",
           total_size_per_alloc, allocation_unit, overhead_per_alloc);

    // Let's fill up most of the memory, leaving just enough space for our test
    size_t target_usage = MEMORY_SIZE * 0.95; // Use 95% of memory

    while (count < MAX_ALLOCATIONS && total_allocated < target_usage) {

        allocations[count].ptr = freelist2_alloc(&fl, TINY_ALLOCATION);
        freelist2_set_allocation_owner(&fl, allocations[count].ptr, &allocations[count].ptr);

        if (allocations[count].ptr != NULL) {
            allocations[count].size = TINY_ALLOCATION;
            allocations[count].pattern = count % 256;
            allocations[count].is_active = true;
            freelist2_fill_memory_pattern(allocations[count].ptr, TINY_ALLOCATION, allocations[count].pattern);
            total_allocated += total_size_per_alloc;
            count++;

            if (count % 500 == 0) {
                printf("\tMade %d allocations, memory used: %zu bytes (%.1f%%)\n",
                       count, total_allocated,
                       (total_allocated * 100.0) / MEMORY_SIZE);
            }
        }
        else
        {
            break;
        }
    }

    printf("\n\tInitial state:\n");
    printf("\t- Created %d allocations\n", count);
    printf("\t- Total allocated: %zu bytes (%.1f%%)\n",
           total_allocated, (total_allocated * 100.0) / MEMORY_SIZE);
    printf("\t- Free space: %zu bytes\n", freelist2_remaining_space(&fl));

    // Free alternating blocks to create fragmentation
    int freed_count = 0;
    for (int i = 0; i < count; i += 2) {
        freelist2_verify_memory_contents(allocations[i].ptr, allocations[i].size, allocations[i].pattern);
        freelist2_free(&fl, allocations[i].ptr);
        allocations[i].is_active = false;
        freed_count++;
    }

    size_t space_after_free = freelist2_remaining_space(&fl);
    printf("\n\tAfter freeing %d blocks:\n", freed_count);
    printf("\t- Free space: %zu bytes\n", space_after_free);

    // Now try to allocate half of our total free space - this should fail due to fragmentation
    size_t large_size = space_after_free - sizeof(Freelist2_Allocation_Header) - DEFAULT_ALIGNMENT;
    void *test_ptr = freelist2_alloc(&fl, large_size);
    bool ownerSet = freelist2_set_allocation_owner(&fl, test_ptr, &test_ptr);
    bool allocation_failed = (test_ptr == NULL && ownerSet == false);
    assert(allocation_failed && "Large allocation should fail when memory is fragmented");

    // Defragment and try again
    freelist2_defragment(&fl);

    // The same allocation should now succeed
    test_ptr = freelist2_alloc(&fl, large_size);
    freelist2_set_allocation_owner(&fl, test_ptr, &test_ptr);
    if (test_ptr == NULL) {
        printf("\tFailed to allocate %zu bytes after defragmentation!\n", large_size);
        assert(false && "Post-defragmentation allocation should succeed");
    }

    printf("\n\tSuccessfully allocated %zu bytes after defragmentation\n", large_size);

    // Verify remaining allocations are still intact
    int verified = 0;
    for (int i = 1; i < count; i += 2) {
        if (allocations[i].is_active) {
            freelist2_verify_memory_contents(allocations[i].ptr, allocations[i].size,
                                 allocations[i].pattern);
            verified++;
        }
    }

    printf("\tVerified %d original allocations remain intact\n", verified);

    free(fl.data);
}

void freelist2_test_realloc() {
    const size_t POOL_SIZE = 1024;
    const size_t DEFAULT_ALIGN = 8;
    // Test 1: Basic realloc with size increase
    {
        Freelist2 fl;
        freelist2_init(&fl, malloc(POOL_SIZE), POOL_SIZE, DEFAULT_ALIGN);
        char *a = freelist2_alloc(&fl, 32);
        freelist2_set_allocation_owner(&fl, a, (void **)&a);
        assert(a != NULL);
        memset(a, 0xA, 32);
        void *original_ptr = a;
        a = freelist2_realloc(&fl, a, 64);
        assert(a != NULL);
        // Verify original data is preserved
        for (int i = 0; i < 32; i++) {
            assert(a[i] == 0xA);
        }
        // Verify we can write to the expanded region
        memset(a + 32, 0xB, 32);
        freelist2_free_all(&fl);
    }
    // Test 2: Realloc with size decrease
    {
        Freelist2 fl;
        freelist2_init(&fl, malloc(POOL_SIZE), POOL_SIZE, DEFAULT_ALIGN);
        SHALLOC_MANAGED(fl, 64, a, char);
        memset(a, 0xA, 64);
        a = freelist2_realloc(&fl, a, 32);
        assert(a != NULL);
        // Verify first 32 bytes are preserved
        for (int i = 0; i < 32; i++) {
            assert(a[i] == 0xA);
        }
        freelist2_free_all(&fl);
    }
    // Test 3: Realloc between other allocations
    {
        Freelist2 fl;
        freelist2_init(&fl, malloc(POOL_SIZE), POOL_SIZE, DEFAULT_ALIGN);
        SHALLOC_MANAGED(fl, 32, a, char);
        SHALLOC_MANAGED(fl, 32, b, char);
        SHALLOC_MANAGED(fl, 32, c, char);
        memset(a, 0xA, 32);
        memset(b, 0xB, 32);
        memset(c, 0xC, 32);
        // Reallocate middle block
        b = freelist2_realloc(&fl, b, 64);
        assert(b != NULL);
        // Verify surrounding blocks are intact
        for (int i = 0; i < 32; i++) {
            assert(a[i] == 0xA);
            assert(c[i] == 0xC);
        }
        freelist2_free_all(&fl);
    }
    // Test 4: Realloc to same size (should be no-op)
    {
        Freelist2 fl;
        freelist2_init(&fl, malloc(POOL_SIZE), POOL_SIZE, DEFAULT_ALIGN);
        SHALLOC_MANAGED(fl, 32, a, char);
        void *original_ptr = a;
        memset(a, 0xA, 32);
        a = freelist2_realloc(&fl, a, 32);
        assert(a != NULL);
        assert(a == original_ptr);  // Should return same pointer
        for (int i = 0; i < 32; i++) {
            assert(a[i] == 0xA);
        }
        freelist2_free_all(&fl);
    }
    // Test 5: Edge case - realloc to zero size
    {
        Freelist2 fl;
        freelist2_init(&fl, malloc(POOL_SIZE), POOL_SIZE, DEFAULT_ALIGN);
        SHALLOC_MANAGED(fl, 32, a, char);
        a = freelist2_realloc(&fl, a, 0);
        assert(a == NULL);  // Should behave like free
        freelist2_free_all(&fl);
    }
    // Test 6: Edge case - realloc null pointer (should behave like alloc)
    {
        Freelist2 fl;
        freelist2_init(&fl, malloc(POOL_SIZE), POOL_SIZE, DEFAULT_ALIGN);
        char *a = NULL;
        a = freelist2_realloc(&fl, a, 32);
        assert(a != NULL);
        memset(a, 0xA, 32);  // Should be able to write to memory
        freelist2_free_all(&fl);
    }
    // Test 7: Realloc near pool size limit
    {
        Freelist2 fl;
        freelist2_init(&fl, malloc(POOL_SIZE), POOL_SIZE, DEFAULT_ALIGN);
        SHALLOC_MANAGED(fl, POOL_SIZE/2, a, char);
        assert(a != NULL);
        // Try to realloc to a size that's too large
        char *b = freelist2_realloc(&fl, a, POOL_SIZE * 2);
        assert(b == NULL);  // Should fail gracefully
        // Original allocation should still be valid
        assert(a != NULL);
        freelist2_free_all(&fl);
    }
    printf("All realloc tests completed successfully!\n");
}

void
freelist2_test_managed()
{
    freelist2_test_alternating_patterns();
    freelist2_test_random_sizes();
    freelist2_test_maximum_fragmentation();
    freelist2_test_alignments();
    freelist2_test_fibonacci_pattern();
    freelist2_test_prime_sizes();
    freelist2_test_pyramid_pattern();
    freelist2_test_extreme_alignments();
    freelist2_test_interlaced_pattern();
    freelist2_test_fragmentation_recovery();
    freelist2_test_realloc();
}

void
freelist2_unit_tests()
{
    freelist2_alignment_tests();
    freelist2_realloc_tests();
    freelist2_fragmentation_tests();
    freelist2_test_managed();
}
#endif
#endif
#endif
