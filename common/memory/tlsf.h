#pragma once


#include <Windows.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include "memory.h"

namespace Tlsf
{

struct Allocation
{
    uint32_t offset;
    uint32_t nodeIndex;
};

class Allocator
{
    typedef uint32_t NodePtr_t;
    static constexpr NodePtr_t NULLPTR = 0xffffffff;
    static constexpr NodePtr_t INVALID_INDEX = 0xffffffff;

    static constexpr uint32_t L1_BIN_COUNT = 32; /* level-1 bins */
    static constexpr uint32_t L2_LOG2_BINCOUNT = 3;
    /* 8 lvl-2 bins for each l1 range = 2^(L2_LOG2_BINCOUNT) = 2^3 = 8 */
    static constexpr uint32_t L2_BIN_COUNT = (1 << L2_LOG2_BINCOUNT);
    static constexpr uint32_t L2_MASK = L2_BIN_COUNT - 1;
    static constexpr uint32_t TOTAL_BIN_COUNT = L1_BIN_COUNT * L2_BIN_COUNT;
    /* each l2Bitmask has 8 bits, one bit for each freelist for that size class. */
    static constexpr uint32_t L2_BITMASK_COUNT = TOTAL_BIN_COUNT / 8;

    static constexpr uint32_t MIN_SIZE_ALLOWED = 8; /* minimum 8 bytes */

  private:

    struct Node
    {
        /* offset from the start of our memory store. */
        uint32_t offset = 0;
        /* size associated with this block. */
        uint32_t size = 0;
        /* linked-list pointers for freelists. */
        NodePtr_t next = NULLPTR;
        NodePtr_t prev = NULLPTR;
        /* address-ordered pointers to be used when merging freeblocks. */
        NodePtr_t aoPrevious = NULLPTR;
        NodePtr_t aoNext = NULLPTR;

        bool allocated = false;
    };

    /* returns the bit position of the most significant bit which is set. Floor(Log2(mask)) */
    inline uint32_t msbIndex(uint32_t mask)
    {
        assert(mask != 0);

        DWORD result = 0;
        _BitScanReverse(&result, mask);
        return result;
    }

    /* returns the bit position of the least significant bit which is set. */
    inline uint32_t lsbIndex(uint32_t mask)
    {
        assert(mask != 0);

        DWORD result;
        _BitScanForward(&result, mask);
        return result;
    }

    /* get the size class of the given size sent in here.  */
    uint32_t indexRounddown(uint32_t size)
    {
        assert(size >= 8);

        /// floor(log2(size))
        uint32_t l1Index = msbIndex(size);
        assert(l1Index >= 3 && l1Index < L1_BIN_COUNT);

        /// l2Index is the subdivision within the l1 range.
        uint32_t diff = size - (1 << l1Index);
        uint32_t l2Index = (diff >> (l1Index - L2_LOG2_BINCOUNT));
        assert(l2Index < L2_BIN_COUNT);

        // NOTE: minimum alloc size is 8 which is 2^3. not storing bytes less than 8.
        l1Index -= 3;

        uint32_t freelistIndex = (l1Index << L2_LOG2_BINCOUNT) + l2Index;
        return freelistIndex;
    }

    /*
     * Round up the size to guarantee the first block in the freelist is guaranteed is >= size and return the index
     * of that freelist.
     */
    uint32_t indexRoundup(uint32_t size)
    {
        assert(size >= 8);

        /// IMPORTANT: This roundup is key to tlsf O(1) implementation. If we directly mapped size to a bin without
        /// bin, the head of the freelist on that index might have a smaller freeblock than the size requested. We
        /// have to make sure the head of that freelist is always >= size so it will always be safe to pick the
        /// first one always.
        /// For example: for size=1000, msb_index(1000)=9; 2^9=512; 2^10=1024
        /// so, 1 << (9-3) - 1= 1 << 6 -1 = 64-1=63. so `size` = 1063.
        ///
        /* round-up NOTE: We are snapping size to the next TLSF bucket boundary. */
        size += (1 << (msbIndex(size) - L2_LOG2_BINCOUNT)) - 1;

        uint32_t l1Index = msbIndex(size); // floor(log2(size))
        assert(l1Index >= 3 && l1Index < L1_BIN_COUNT);

        uint32_t diff = size - (1 << l1Index);
        uint32_t l2Index = (diff >> (l1Index - L2_LOG2_BINCOUNT));
        assert(l2Index < L2_BIN_COUNT);

        // NOTE: minimum alloc size is 8 which is 2^3. not storing bytes less than 8.
        l1Index -= 3;

        uint32_t freelistIndex = (l1Index << L2_LOG2_BINCOUNT) + l2Index;
        return freelistIndex;
    }

    inline uint32_t getFreenodeIndex()
    {
        uint32_t freeNodeIndex = m_emptyNodeStack[m_emptyNodeStackTop--];
        if (m_emptyNodeStackTop < 0) {
            assert(!"All freenodes have been consumed!");
        }
        return freeNodeIndex;
    }

    inline void addFreenode(uint32_t nodeIndex)
    {
        assert(m_emptyNodeStackTop < m_maxAllocs-1);
        m_emptyNodeStack[++m_emptyNodeStackTop] = nodeIndex;
    }

    /* get the freelist index corresponding to the l1 and l2 indices passed in here.
     * suitable here means we need to find the a freeblock correponding to the indices passed in
     * OR look for a bigger one in the upper levels.
     * IMPORTANT: TLSF avoids using loops while searching. the search here is O(1)
     */
    void findSuitableFreelist(uint32_t& l1Index, uint32_t& l2Index, uint32_t &freelistIndex)
    {
        /* see if there is a freeblock in this l1. */
        uint32_t mask = 1u << l1Index;
        if ((m_l1Bitmask & mask) == 0) {
            /// No freeblock in this l1. jump to the next larger level l1.
            /// IMPORTANT: This is key for searching in O(1). We dont run a loop we get the first free l1 level.
            /// The bit scan does it all in one go.
            uint32_t b = m_l1Bitmask >> (l1Index + 1);

            /* no memory */
            if (!b) {
                assert(!"INVALID_INDEX");
                freelistIndex = INVALID_INDEX;
                return;
            }

            /// update l1 index to the new one that has freeblocks with the requested size.
            uint32_t i = lsbIndex(b)+1;
            l1Index += i;
            /// we are looking at a higher l1 level than sent in here. We should look at the first
            /// l2 bin at that higher level.
            l2Index = 0;
        }
        
        /* find a suitable l2 bin within this l1. */
        uint8_t l2Mask = m_l2Bitmasks[l1Index];
        /// check if there is a freeblock in this l2 bin.
        if ((l2Mask & (1u << l2Index)) == 0) {
            /// no block in this l2 bin. Look to the next one by shifting right.
            uint32_t b = l2Mask >> (l2Index+1);
            /* this should be guaranteed to be non-zero since the l1_index was set. */
            if (b == 0) {
                assert(!"Invalid Path. l2Mask should not be zero with l1 range being set above.");
                freelistIndex = INVALID_INDEX;
                return;
            }
            /// update l2 index to the new one that has freeblocks with the requested size.
            uint32_t i = lsbIndex(b)+1;
            l2Index += i;
        }

        freelistIndex = (l1Index << L2_LOG2_BINCOUNT) + l2Index;
    }

    /* insert a freeblock of the given size into the head of the appropriate freelist and
       update the bitmasks. */
    uint32_t insertNode(uint32_t size, uint32_t offset)
    {
        /// Freeblocks belong to a size class that does not exceed their true size i.e. the size passed in here.
        uint32_t freelistIndex = indexRounddown(size);

        NodePtr_t headNodeIndex = m_freelistHeads[freelistIndex];
        uint32_t newNodeIndex = getFreenodeIndex();
        Node& newNode = m_nodes[newNodeIndex];
        // place new_node at the top of the freelist.
        newNode.next = headNodeIndex;
        newNode.prev = NULLPTR;
        newNode.size = size;
        newNode.offset = offset;

        if (headNodeIndex != NULLPTR) {
            Node& headNode = m_nodes[headNodeIndex];
            // set prev ptr of the old_head to the new_head.
            headNode.prev = newNodeIndex;
        } else {
            /**
             * NOTE: since the freelist was empty, the bitmasks would've been zeroed out. Set them
             * accordingly to show that this specific size-class's freelist has a free block to
             * accomodate future freeblock searches in this size-class.
             */
            uint32_t l1Index = freelistIndex >> L2_LOG2_BINCOUNT;
            uint32_t l2Index = freelistIndex & L2_MASK;

            m_l1Bitmask |= (1u << l1Index);
            /// there are 8 sizeclasses per l1. 8 sizeclass make up 8 bits for the l2Bitmask.
            m_l2Bitmasks[l1Index] |= (1u << l2Index);
        }

        /* set the new head to be the new_node */
        m_freelistHeads[freelistIndex] = newNodeIndex;

        return newNodeIndex;
    }

    void removeNode(uint32_t nodeIndex)
    {
        Node &node = m_nodes[nodeIndex];

        /* remove node from it's current housing freelist. */
        uint32_t index = indexRounddown(node.size);
        if (node.prev == NULLPTR)
        {
            if (node.next != NULLPTR) {
                /// make the next node the head of the freelist.
                m_nodes[node.next].prev = NULLPTR;
                m_freelistHeads[index] = node.next;

                /// clear the node's next flag.
                node.next = NULLPTR;
            } else {
                /* node is the only node in it's freelist. */

                /// remove the node.
                m_freelistHeads[index] = NULLPTR;
                /// clear out the bitmasks.
                uint32_t l1Index = index >> L2_LOG2_BINCOUNT;
                uint32_t l2Index = index & L2_MASK; /// l2Index = index % 8
                m_l2Bitmasks[l1Index] &= ~(1 << l2Index);
                if (m_l2Bitmasks[l1Index] == 0) {
                    m_l1Bitmask &= ~(1 << l1Index);
                }
            }
        }
        else
        {
            /// there is a node previous to the node in it's freelist.
            if (node.next != NULLPTR) {
                m_nodes[node.next].prev = node.prev;
            }
            m_nodes[node.prev].next = node.next;

            node.prev = NULLPTR;
        }

        /// the node has been removed from it's freelist. It is empty so we
        /// got to add it to the freestore(stack) of empty nodes
        m_emptyNodeStack[++m_emptyNodeStackTop] = nodeIndex;
    }

  public:
    Allocation allocate(uint32_t size)
    {
        if (size == 0) {
            assert(!"Size 0 is like free that needs the mem pointer! Not applicable here!");
        }
        if (size < MIN_SIZE_ALLOWED) {
            size = MIN_SIZE_ALLOWED;
        }

        /*
         * rounding up while finding the size_class is what makes this allocator O(1).
         * Rounded up to a size class boundary so that the freelist head freeblock size is always guaranteed to be
         * enough for the size requested. Hence O(1).
         */
        uint32_t candidateFreelistIndex = indexRoundup(size);
        /// l1_index is (index / 8)
        uint32_t l1Index = candidateFreelistIndex >> L2_LOG2_BINCOUNT;
        /// l2 index is (index % 8)
        uint32_t l2Index = candidateFreelistIndex & L2_MASK;

        /// Get the index of the freelist in the freelist array which has the apt freeblock to satisfy this
        /// memory request.
        uint32_t freelistIndex = INVALID_INDEX;
        findSuitableFreelist(l1Index, l2Index, freelistIndex);
        if (freelistIndex == INVALID_INDEX) { assert(!"No memory left"); }

        NodePtr_t headNodeIndex = m_freelistHeads[freelistIndex];
        /// the headnode_index shouldn't be a nullptr since freelist_index above was not an INVALID_INDEX.
        assert(headNodeIndex != INVALID_INDEX && headNodeIndex < TOTAL_BIN_COUNT);
        Node *headNode = m_nodes + headNodeIndex;
        assert(headNode->size >= size);

        /// remove the headNode. make it's next one the head of this freelist and
        /// update its prev pointer to null.
        NodePtr_t nextNodeIndex = headNode->next;
        /// set the next node to this as the head -> This removes the head node.
        m_freelistHeads[freelistIndex] = nextNodeIndex;
        if (nextNodeIndex != NULLPTR) {
            /// next node is not a nullptr. It's a valid node.
            m_nodes[nextNodeIndex].prev = NULLPTR;
        } else {
            /// unset the l2 bin bit.
            m_l2Bitmasks[l1Index] &= ~(1 << l2Index);
            /// if the whole l2 bin is empty(all 8 bins), then the whole range of 8 sizes that are
            /// supposed to be in an l1 bin is empty, so we gotta set the l1 bin as well.
            if (m_l2Bitmasks[l1Index] == 0) {
                m_l1Bitmask &= ~(1 << l1Index);
            }
        }

        assert(headNode->size >= size);
        uint32_t remainingSize = headNode->size - size;

        // TODO: roundup allocSize to avoid splitting blocks unless absolutely necessary.
        if (remainingSize >= MIN_SIZE_ALLOWED)
        {
            /// insert freeblock of size 'remaining_size' into the apt freelist.
            uint32_t newNodeIndex = insertNode(remainingSize, headNode->offset + size);

            /// decrement the amount of remainingSize from the current head of the freelist.
            assert(headNode->size > size);
            headNode->size -= remainingSize;

            /// set aoNext and aoPrev pointers so that merging freeblocks can be done during
            /// free.
            Node& newNode = m_nodes[newNodeIndex];
            if (headNode->aoNext != NULLPTR) {
                m_nodes[headNode->aoNext].aoPrevious = newNodeIndex;
            }

            newNode.aoPrevious = headNodeIndex;
            newNode.aoNext = headNode->aoNext;

            headNode->aoNext = newNodeIndex;
        }

        headNode->allocated = true;
        return
        {
            .offset = headNode->offset,
            .nodeIndex = headNodeIndex
        };
    }

    void free(Allocation allocation)
    {
        /// basically what we have to do here is we have to find the node that is being freed.
        /// That node has a size associated

        Node &currentNode = m_nodes[allocation.nodeIndex];
        if (!currentNode.allocated) {
            assert(!"Double Free");
            return;
        }

        /// check to see if we can merge with the previous address-ordered node.
        if (currentNode.aoPrevious != NULLPTR &&
            !m_nodes[currentNode.aoPrevious].allocated)
        {
            /// remove the previous node to currentNode from it's housing freelist since it's going
            /// to be merged with the currentNode.
            removeNode(currentNode.aoPrevious);

            Node &prevNode = m_nodes[currentNode.aoPrevious];
            /// do the actual merging of the current node and its addres-ordered previous node.
            currentNode.size += prevNode.size;

            assert(prevNode.offset < m_nodes[allocation.nodeIndex].offset);
            currentNode.offset = prevNode.offset;

            /// the aoPrev of the prevNode should now be previous to the merged node.
            currentNode.aoPrevious = prevNode.aoPrevious;
        }

        /// check to see if we can merge with the next address-ordered node.
        if (currentNode.aoNext != NULLPTR &&
            !m_nodes[currentNode.aoNext].allocated)
        {
            removeNode(currentNode.aoNext);

            Node &nextNode = m_nodes[currentNode.aoNext];
            currentNode.size += nextNode.size;
            assert(nextNode.offset < m_nodes[allocation.nodeIndex].offset);

            /// the aoNext of the nextNode should be aoNext of the merged node.
            currentNode.aoNext = nextNode.aoNext;
        }

        uint32_t aoPreviousCached = currentNode.aoPrevious;
        uint32_t aoNextCached = currentNode.aoNext;

        m_emptyNodeStack[++m_emptyNodeStackTop] = allocation.nodeIndex;

        /// Now we need to insert the merged node into the appropriate freelist.
        uint32_t insertedNodeIndex = insertNode(currentNode.size, currentNode.offset);

        /// Just checking my brain might have farted. :P
        assert(allocation.nodeIndex == insertedNodeIndex);

        // TODO: insertNode() should return the same node from the stack that we pushed. So aoNext and aoPrev
        //       should be the same now as well.
        assert(m_nodes[insertedNodeIndex].aoPrevious == aoPreviousCached &&
               m_nodes[insertedNodeIndex].aoNext == aoNextCached);
    }

    Allocator(uint32_t maxAllocs = 4096)
        : m_maxAllocs(maxAllocs)
    {
        m_backBuffer = VirtualAlloc((LPVOID)TERABYTES(4), GIGABYTES(4), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!m_backBuffer) {
            puts("VirtualAlloc failed!");
        }

        memset(m_l2Bitmasks, 0, sizeof(uint8_t) * L2_BITMASK_COUNT);
        /* all freelist head_ptrs are NULLPTR's */
        memset(m_freelistHeads, 0xff, TOTAL_BIN_COUNT*sizeof(NodePtr_t));

        // VirtualAlloc should return the lpAddress we sent in. ( xX This can fail Xx )
        // assert((uint64_t)back_buffer_ == TERABYTES(4));

        m_nodes = new Node[m_maxAllocs];
        m_emptyNodeStack = new uint32_t[m_maxAllocs];

        for (uint32_t i = 0; i < m_maxAllocs; ++i) {
            m_emptyNodeStack[i] = m_maxAllocs - i - 1;
        }
        m_emptyNodeStackTop = m_maxAllocs - 1;

        insertNode(GIGABYTES(4)-1, 0);
    }

  private:
    void *m_backBuffer;

    /* 32 bits where each bit corresponds to one lvl-1 bin. */
    uint32_t m_l1Bitmask = 0u;
    /* each lvl-2 bin has length=8. 1 bit for each so uint8_t */
    uint8_t  m_l2Bitmasks[L2_BITMASK_COUNT];
    NodePtr_t  m_freelistHeads[TOTAL_BIN_COUNT];

    uint32_t m_maxAllocs;

    /* pool of total nodes that can be allocated from for new allocation requests. */
    Node *m_nodes;
    /* the stack of free nodes. */
    uint32_t *m_emptyNodeStack;
    uint32_t  m_emptyNodeStackTop;
};

/**
 * EXPLANATION:
 * The number of freelists(heads) is 256 (32 * 8) where and can be thought as a 2 dimensional array.
 * freelists[32][8]: each l1 is a log power of 2 size and each of these are further subdivided into 8 distinct size
 * classes.
 * there is one 32 bit bitmask for l1. and l2 bitmasks for each.
 * if a bit j is set in l2_bitmasks[i], then the l1 level is [2^i, 2^(i+1)-1]
 * and the size class inside of this is in the range (2^i + j*(2^(i-3))).
 * This basically means that there is a freelist with an empty freeblock which can serve the memory request at this
 * index.
 **
 *
 */
}

