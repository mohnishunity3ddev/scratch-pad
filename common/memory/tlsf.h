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

    static constexpr uint32_t L1_BIN_COUNT      = 32;
    static constexpr uint32_t L2_LOG2_BINCOUNT  = 3;
    static constexpr uint32_t L2_BIN_COUNT      = (1 << L2_LOG2_BINCOUNT);
    static constexpr uint32_t L2_MASK           = L2_BIN_COUNT - 1;
    static constexpr uint32_t TOTAL_BIN_COUNT   = L1_BIN_COUNT * L2_BIN_COUNT;
    static constexpr uint32_t L2_BITMASK_COUNT  = TOTAL_BIN_COUNT / 8;
    static constexpr uint32_t MIN_SIZE_ALLOWED  = 8;

  private:
    struct Node
    {
        uint32_t  offset = INVALID_INDEX;
        uint32_t  size = 0;
        NodePtr_t next = NULLPTR;
        NodePtr_t prev = NULLPTR;
        NodePtr_t aoPrevious = NULLPTR;
        NodePtr_t aoNext = NULLPTR;

        bool allocated = false;
    };

    inline uint32_t msbIndex(uint32_t mask);
    inline uint32_t lsbIndex(uint32_t mask);
    
    uint32_t indexRounddown(uint32_t size);
    uint32_t indexRoundup(uint32_t size);
    inline uint32_t getFreenodeIndex();
    inline void addNodeToStore(uint32_t nodeIndex);
    void findSuitableFreelist(uint32_t& l1Index, uint32_t& l2Index, uint32_t &freelistIndex);
    uint32_t insertNode(uint32_t size, uint32_t offset);

    void removeNode(uint32_t nodeIndex);

  public:
    Allocation allocate(uint32_t size);
    void free(Allocation allocation);

    void validate() const;

    Allocator(uint32_t maxAllocs = 4096);
    ~Allocator();

  private:
    void *m_backBuffer;

    uint32_t   m_l1Bitmask = 0u;
    uint8_t    m_l2Bitmasks[L2_BITMASK_COUNT];
    NodePtr_t  m_freelistHeads[TOTAL_BIN_COUNT];
    uint32_t   m_maxAllocs;
    Node      *m_nodes;
    uint32_t  *m_emptyNodeStack;
    uint32_t   m_emptyNodeStackTop;
};

}