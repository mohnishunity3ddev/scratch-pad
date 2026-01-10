#include "tlsf.h"
#include "memory.h"
#include <cstring>

namespace Tlsf
{
/* returns the bit position of the most significant bit which is set. Floor(Log2(mask)) */
inline uint32_t
Allocator::msbIndex(uint32_t mask)
{
    assert(mask != 0);

    DWORD result = 0;
    _BitScanReverse(&result, mask);
    return result;
}

/* returns the bit position of the least significant bit which is set. */
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

    uint32_t diff = size - (1 << l1Index);
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

inline uint32_t
Allocator::getFreenodeIndex()
{
    uint32_t freeNodeIndex = m_emptyNodeStack[m_emptyNodeStackTop--];
    if (m_emptyNodeStackTop < 0)
    {
        assert(!"All freenodes have been consumed!");
    }
    return freeNodeIndex;
}

inline void
Allocator::addFreenode(uint32_t nodeIndex)
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
    if ((l2Mask & (1u << l2Index)) == 0)
    {
        uint32_t b = l2Mask >> (l2Index + 1);
        if (b == 0)
        {
            assert(!"Invalid Path. l2Mask should not be zero with l1 range being set above.");
            freelistIndex = INVALID_INDEX;
            return;
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

    uint32_t index = indexRounddown(node.size);
    if (node.prev == NULLPTR)
    {
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
            if (m_l2Bitmasks[l1Index] == 0) {
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

    m_emptyNodeStack[++m_emptyNodeStackTop] = nodeIndex;
}

Allocation
Allocator::allocate(uint32_t size)
{
    if (size == 0) { assert(!"Size 0 is like free!"); }
    if (size < MIN_SIZE_ALLOWED) { size = MIN_SIZE_ALLOWED; }

    uint32_t candidateFreelistIndex = indexRoundup(size);
    uint32_t l1Index = candidateFreelistIndex >> L2_LOG2_BINCOUNT;
    uint32_t l2Index = candidateFreelistIndex & L2_MASK;

    uint32_t freelistIndex = INVALID_INDEX;
    findSuitableFreelist(l1Index, l2Index, freelistIndex);
    if (freelistIndex == INVALID_INDEX) {
        assert(!"No memory left");
    }

    NodePtr_t headNodeIndex = m_freelistHeads[freelistIndex];
    assert(headNodeIndex != INVALID_INDEX && headNodeIndex < TOTAL_BIN_COUNT);
    Node *headNode = m_nodes + headNodeIndex;
    assert(headNode->size >= size);

    NodePtr_t nextNodeIndex = headNode->next;
    m_freelistHeads[freelistIndex] = nextNodeIndex;
    if (nextNodeIndex != NULLPTR)
    {
        m_nodes[nextNodeIndex].prev = NULLPTR;
    }
    else
    {
        m_l2Bitmasks[l1Index] &= ~(1 << l2Index);
        if (m_l2Bitmasks[l1Index] == 0) {
            m_l1Bitmask &= ~(1 << l1Index);
        }
    }

    assert(headNode->size >= size);
    uint32_t remainingSize = headNode->size - size;

    if (remainingSize >= MIN_SIZE_ALLOWED)
    {
        uint32_t newNodeIndex = insertNode(remainingSize, headNode->offset + size);

        assert(headNode->size > size);
        headNode->size -= remainingSize;

        Node &newNode = m_nodes[newNodeIndex];
        if (headNode->aoNext != NULLPTR)
        {
            m_nodes[headNode->aoNext].aoPrevious = newNodeIndex;
        }

        newNode.aoPrevious = headNodeIndex;
        newNode.aoNext = headNode->aoNext;
        
        headNode->aoNext = newNodeIndex;
    }

    headNode->allocated = true;
    return {.offset = headNode->offset, .nodeIndex = headNodeIndex};
}

void
Allocator::free(Allocation allocation)
{

    Node &currentNode = m_nodes[allocation.nodeIndex];
    if (!currentNode.allocated)
    {
        assert(!"Double Free? or a valid allocated node was not marked as allocated.");
        return;
    }

    if (currentNode.aoPrevious != NULLPTR && !m_nodes[currentNode.aoPrevious].allocated)
    {
        removeNode(currentNode.aoPrevious);

        Node &prevNode = m_nodes[currentNode.aoPrevious];
        currentNode.size += prevNode.size;

        assert(prevNode.offset < m_nodes[allocation.nodeIndex].offset);
        currentNode.offset = prevNode.offset;

        currentNode.aoPrevious = prevNode.aoPrevious;
    }

    if (currentNode.aoNext != NULLPTR && !m_nodes[currentNode.aoNext].allocated)
    {
        removeNode(currentNode.aoNext);

        Node &nextNode = m_nodes[currentNode.aoNext];
        currentNode.size += nextNode.size;
        assert(nextNode.offset < m_nodes[allocation.nodeIndex].offset);

        currentNode.aoNext = nextNode.aoNext;
    }

    uint32_t aoPreviousCached = currentNode.aoPrevious;
    uint32_t aoNextCached = currentNode.aoNext;

    m_emptyNodeStack[++m_emptyNodeStackTop] = allocation.nodeIndex;

    uint32_t insertedNodeIndex = insertNode(currentNode.size, currentNode.offset);

    assert(allocation.nodeIndex == insertedNodeIndex);

    // TODO: insertNode() should return the same node from the stack that we pushed. So aoNext and aoPrev
    //       should be the same now as well.
    assert(m_nodes[insertedNodeIndex].aoPrevious == aoPreviousCached &&
           m_nodes[insertedNodeIndex].aoNext == aoNextCached);
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

}