#include "tlsf.h"
#include "memory.h"
#include <cstring>
#include <iostream>
#include <memoryapi.h>

namespace Tlsf
{

inline uint32_t
Allocator::msbIndex(uint32_t mask)
{
    assert(mask != 0);

    DWORD result = 0;
    _BitScanReverse(&result, mask);
    return result;
}

inline uint32_t
Allocator::lsbIndex(uint32_t mask)
{
    assert(mask != 0);

    DWORD result;
    _BitScanForward(&result, mask);
    return result;
}

uint32_t
Allocator::indexRounddown(uint32_t size)
{
    assert(size >= 8);

    uint32_t l1Index = msbIndex(size);
    assert(l1Index >= 3 && l1Index < L1_BIN_COUNT);

    uint32_t diff = size - (1u << l1Index);
    uint32_t l2Index = (diff >> (l1Index - L2_LOG2_BINCOUNT));
    assert(l2Index < L2_BIN_COUNT);

    // NOTE: minimum alloc size is 8 which is 2^3. not storing bytes less than 8.
    l1Index -= 3;

    uint32_t freelistIndex = (l1Index << L2_LOG2_BINCOUNT) + l2Index;
    return freelistIndex;
}

uint32_t
Allocator::indexRoundup(uint32_t size)
{
    assert(size >= 8);

    /* round-up NOTE: We are snapping size to the next TLSF bucket boundary. */
    uint32_t sz = size + ((1 << (msbIndex(size) - L2_LOG2_BINCOUNT)) - 1);

    uint32_t l1Index = msbIndex(sz); // floor(log2(sz))
    assert(l1Index >= 3 && l1Index < L1_BIN_COUNT);

    uint32_t diff = sz - (1 << l1Index);
    uint32_t l2Index = (diff >> (l1Index - L2_LOG2_BINCOUNT));
    assert(l2Index < L2_BIN_COUNT);

    // NOTE: minimum alloc size is 8 which is 2^3. not storing bytes less than 8.
    l1Index -= 3;

    uint32_t freelistIndex = (l1Index << L2_LOG2_BINCOUNT) + l2Index;
    return freelistIndex;
}

inline uint32_t
Allocator::getFreenodeIndex()
{
    if (m_emptyNodeStackTop == 0)
    {
        assert(!"All freenodes have been consumed`!");
    }
    uint32_t freeNodeIndex = m_emptyNodeStack[m_emptyNodeStackTop--];
    return freeNodeIndex;
}

inline void
Allocator::addNodeToStore(uint32_t nodeIndex)
{
    assert(m_emptyNodeStackTop < m_maxAllocs - 1);
    m_emptyNodeStack[++m_emptyNodeStackTop] = nodeIndex;
}

void
Allocator::findSuitableFreelist(uint32_t &l1Index, uint32_t &l2Index, uint32_t &freelistIndex)
{
    uint32_t mask = 1u << l1Index;
    if ((m_l1Bitmask & mask) == 0)
    {
    higher_l1_bin:
        uint32_t b = m_l1Bitmask >> (l1Index + 1);

        if (!b)
        {
            assert(!"INVALID_INDEX");
            freelistIndex = INVALID_INDEX;
            return;
        }

        uint32_t i = lsbIndex(b) + 1;
        l1Index += i;
        l2Index = 0;
    }

    uint8_t l2Mask = m_l2Bitmasks[l1Index];
    if ((l2Mask & (static_cast<uint8_t>(1) << l2Index)) == 0)
    {
        uint32_t b = l2Mask >> (l2Index + 1);
        if (b == 0)
        {
            goto higher_l1_bin;
        }
        uint32_t i = lsbIndex(b) + 1;
        l2Index += i;
    }

    freelistIndex = (l1Index << L2_LOG2_BINCOUNT) + l2Index;
}

uint32_t
Allocator::insertNode(uint32_t size, uint32_t offset)
{
    uint32_t freelistIndex = indexRounddown(size);

    NodePtr_t headNodeIndex = m_freelistHeads[freelistIndex];
    uint32_t newNodeIndex = getFreenodeIndex();
    Node &newNode = m_nodes[newNodeIndex];
    newNode.next = headNodeIndex;
    newNode.prev = NULLPTR;
    newNode.size = size;
    newNode.offset = offset;

    if (headNodeIndex != NULLPTR)
    {
        Node &headNode = m_nodes[headNodeIndex];
        headNode.prev = newNodeIndex;
    }
    else
    {
        uint32_t l1Index = freelistIndex >> L2_LOG2_BINCOUNT;
        uint32_t l2Index = freelistIndex & L2_MASK;

        m_l1Bitmask |= (1u << l1Index);
        m_l2Bitmasks[l1Index] |= (1u << l2Index);
    }

    m_freelistHeads[freelistIndex] = newNodeIndex;

    return newNodeIndex;
}

void
Allocator::removeNode(uint32_t nodeIndex)
{
    Node &node = m_nodes[nodeIndex];
    /// NOTE: nodes with offset = NULLPTR are supposed to be invalid and already present in the empty node stack.
    assert(!node.allocated && node.offset != NULLPTR &&
           "this function is meant to remove only non-allocated free nodes.");

    uint32_t index = indexRounddown(node.size);
    if (node.prev == NULLPTR)
    {
        /// It is the first node its freelist.
        assert(m_freelistHeads[index] == nodeIndex);
        if (node.next != NULLPTR)
        {
            m_nodes[node.next].prev = NULLPTR;
            m_freelistHeads[index] = node.next;
            node.next = NULLPTR;
        }
        else
        {
            m_freelistHeads[index] = NULLPTR;
            uint32_t l1Index = index >> L2_LOG2_BINCOUNT;
            uint32_t l2Index = index & L2_MASK;
            m_l2Bitmasks[l1Index] &= ~(1 << l2Index);
            if (m_l2Bitmasks[l1Index] == 0)
            {
                m_l1Bitmask &= ~(1 << l1Index);
            }
        }
    }
    else
    {
        if (node.next != NULLPTR)
        {
            m_nodes[node.next].prev = node.prev;
        }
        m_nodes[node.prev].next = node.next;

        node.prev = NULLPTR;
    }

    /// Reset the node before returning it.
    node.offset = INVALID_INDEX;
    node.size = 0;
    node.next = NULLPTR;
    node.prev = NULLPTR;
    node.aoPrevious = NULLPTR;
    node.aoNext = NULLPTR;
    node.allocated = false;

    /// Return the node to the freestore.
    m_emptyNodeStack[++m_emptyNodeStackTop] = nodeIndex;
}

Allocation
Allocator::allocate(uint32_t size)
{
    if (size == 0)
    {
        assert(!"Size 0 is like free!");
    }
    if (size < MIN_SIZE_ALLOWED)
    {
        size = MIN_SIZE_ALLOWED;
    }

    std::cout << "***********  Allocating " << size << " bytes.  **********\n";

    uint32_t candidateFreelistIndex = indexRoundup(size);
    uint32_t l1Index = candidateFreelistIndex >> L2_LOG2_BINCOUNT;
    uint32_t l2Index = candidateFreelistIndex & L2_MASK;

    uint32_t freelistIndex = INVALID_INDEX;
    findSuitableFreelist(l1Index, l2Index, freelistIndex);

    if (freelistIndex == INVALID_INDEX)
    {
        assert(!"No memory left");
    }

    NodePtr_t headNodeIndex = m_freelistHeads[freelistIndex];
    assert(headNodeIndex != INVALID_INDEX && headNodeIndex < m_maxAllocs);
    Node &headNode = m_nodes[headNodeIndex];
    assert(headNode.size >= size && !headNode.allocated);

    NodePtr_t nextNodeIndex = headNode.next;
    m_freelistHeads[freelistIndex] = nextNodeIndex;
    if (nextNodeIndex != NULLPTR)
    {
        m_nodes[nextNodeIndex].prev = NULLPTR;
    }
    else
    {
        m_l2Bitmasks[l1Index] &= ~(1 << l2Index);
        if (m_l2Bitmasks[l1Index] == 0)
        {
            m_l1Bitmask &= ~(1 << l1Index);
        }
    }

    assert(headNode.size >= size);
    uint32_t remainingSize = headNode.size - size;

    if (remainingSize >= MIN_SIZE_ALLOWED)
    {
        uint32_t newNodeIndex = insertNode(remainingSize, headNode.offset + size);

        assert(headNode.size > size);
        headNode.size -= remainingSize;

        Node &newNode = m_nodes[newNodeIndex];
        if (headNode.aoNext != NULLPTR)
        {
            m_nodes[headNode.aoNext].aoPrevious = newNodeIndex;
        }

        newNode.aoPrevious = headNodeIndex;
        newNode.aoNext = headNode.aoNext;

        headNode.aoNext = newNodeIndex;
    }

    headNode.allocated = true;
    validate();
    return {.offset = headNode.offset, .nodeIndex = headNodeIndex};
}

void
Allocator::free(Allocation allocation)
{
    assert(allocation.nodeIndex != INVALID_INDEX);
    std::cout << "***********  Freeing " << m_nodes[allocation.nodeIndex].size << " bytes at offset "
              << allocation.offset << " *********\n";

    Node &nodeToFree = m_nodes[allocation.nodeIndex];
    if (!nodeToFree.allocated)
    {
        assert(!"Double Free? or a valid allocated node was not marked as allocated.");
        return;
    }

    if (nodeToFree.aoPrevious != NULLPTR && !m_nodes[nodeToFree.aoPrevious].allocated)
    {
        Node prevFreeNeighbor = m_nodes[nodeToFree.aoPrevious];
        removeNode(nodeToFree.aoPrevious);

        nodeToFree.size += prevFreeNeighbor.size;

        assert(prevFreeNeighbor.offset < m_nodes[allocation.nodeIndex].offset);
        nodeToFree.offset = prevFreeNeighbor.offset;

        nodeToFree.aoPrevious = prevFreeNeighbor.aoPrevious;
        if (nodeToFree.aoPrevious != INVALID_INDEX)
        {
            m_nodes[nodeToFree.aoPrevious].aoNext = allocation.nodeIndex;
        }
    }

    if (nodeToFree.aoNext != NULLPTR && !m_nodes[nodeToFree.aoNext].allocated)
    {
        /// cache the next node
        Node nextFreeNeighbor = m_nodes[nodeToFree.aoNext];
        /// remove
        removeNode(nodeToFree.aoNext);

        nodeToFree.size += nextFreeNeighbor.size;
        assert(nextFreeNeighbor.offset > m_nodes[allocation.nodeIndex].offset);

        nodeToFree.aoNext = nextFreeNeighbor.aoNext;
        if (nodeToFree.aoNext != INVALID_INDEX)
        {
            m_nodes[nodeToFree.aoNext].aoPrevious = allocation.nodeIndex;
        }
    }

    NodePtr_t p = nodeToFree.aoPrevious;
    NodePtr_t n = nodeToFree.aoNext;

    addNodeToStore(allocation.nodeIndex);
    uint32_t insertedNodeIndex = insertNode(nodeToFree.size, nodeToFree.offset);

    m_nodes[insertedNodeIndex].aoPrevious = p;
    m_nodes[insertedNodeIndex].aoNext = n;
    m_nodes[insertedNodeIndex].allocated = false;

    validate();
}

void
Allocator::validate() const
{
    uint32_t numAllocs = 0, numFrees = 0;
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "[ALLOCATIONS]: \n";
    for (uint32_t nodeIndex = 0, count = 0; nodeIndex < m_maxAllocs; ++nodeIndex)
    {
        const Node &node = m_nodes[nodeIndex];
        if (node.offset != INVALID_INDEX && node.allocated)
        {
            numAllocs++;
            ++count;
            std::cout << count << ") offset: " << node.offset << ", size: " << node.size
                      << ", nodeIndex: " << nodeIndex << "\n";

            if (node.aoNext != NULLPTR && m_nodes[node.aoNext].offset != NULLPTR)
            {
                assert(m_nodes[node.aoNext].aoPrevious == nodeIndex);
                assert(node.offset + node.size == m_nodes[node.aoNext].offset);
            }
            if (node.aoPrevious != NULLPTR && m_nodes[node.aoPrevious].offset != NULLPTR)
            {
                assert(m_nodes[node.aoPrevious].aoNext == nodeIndex);
                assert(m_nodes[node.aoPrevious].offset + m_nodes[node.aoPrevious].size == node.offset);
            }
        }
    }

    std::cout << "[VALIDATE]: numAllocs = " << numAllocs << "; numFrees = " << numFrees << "\n";
    std::cout << "----------------------------------------------------------------\n";
}

Allocator::Allocator(uint32_t maxAllocs) : m_maxAllocs(maxAllocs)
{
    m_backBuffer = VirtualAlloc((LPVOID)TERABYTES(4), GIGABYTES(4), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!m_backBuffer)
    {
        puts("VirtualAlloc failed!");
    }

    memset(m_l2Bitmasks, 0, sizeof(uint8_t) * L2_BITMASK_COUNT);
    memset(m_freelistHeads, 0xff, TOTAL_BIN_COUNT * sizeof(NodePtr_t));

    m_nodes = new Node[m_maxAllocs];
    m_emptyNodeStack = new uint32_t[m_maxAllocs];

    for (uint32_t i = 0; i < m_maxAllocs; ++i)
    {
        m_emptyNodeStack[i] = m_maxAllocs - i - 1;
    }
    m_emptyNodeStackTop = m_maxAllocs - 1;

    insertNode(GIGABYTES(4) - 1, 0);
}

Allocator::~Allocator()
{
    assert(m_backBuffer != nullptr);
    VirtualFree(m_backBuffer, 0, MEM_RELEASE);
    delete[] m_nodes;
    delete[] m_emptyNodeStack;
}
} // namespace Tlsf