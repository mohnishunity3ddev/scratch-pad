#pragma once


#include <Windows.h>
#include <cassert>
#include <cstdint>

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
    inline uint32_t msbIndex(uint32_t mask);
    /* returns the bit position of the least significant bit which is set. */
    inline uint32_t lsbIndex(uint32_t mask);

    /* get the size class of the given size sent in here.  */
    uint32_t indexRounddown(uint32_t size);

    /*
     * Round up the size to guarantee the first block in the freelist is guaranteed is >= size and return the index
     * of that freelist.
     */
    uint32_t indexRoundup(uint32_t size);

    inline uint32_t getFreenodeIndex();

    inline void addFreenode(uint32_t nodeIndex);

    /* get the freelist index corresponding to the l1 and l2 indices passed in here.
     * suitable here means we need to find the a freeblock correponding to the indices passed in
     * OR look for a bigger one in the upper levels.
     * IMPORTANT: TLSF avoids using loops while searching. the search here is O(1)
     */
    void findSuitableFreelist(uint32_t& l1Index, uint32_t& l2Index, uint32_t &freelistIndex);

    /* insert a freeblock of the given size into the head of the appropriate freelist and
       update the bitmasks. */
    uint32_t insertNode(uint32_t size, uint32_t offset);

    void removeNode(uint32_t nodeIndex);

  public:
    Allocation allocate(uint32_t size);
    void free(Allocation allocation);

    Allocator(uint32_t maxAllocs = 4096);

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

