#define TLSF_INCLUDE_TESTS
#include "tlsf.h"
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <random>


#ifdef TLSF_INCLUDE_TESTS

namespace Tlsf
{

namespace Testing
{

// ============================================================================
// Helper Functions for Testing
// ============================================================================

class AllocatorValidator
{
  private:
    struct AllocationRecord
    {
        uint32_t offset;
        uint32_t size;
        uint32_t nodeIndex;
    };

    std::map<uint32_t, AllocationRecord> m_activeAllocations;
    uint32_t m_totalAllocated = 0;
    uint32_t m_peakAllocated = 0;
    uint32_t m_allocationCount = 0;
    uint32_t m_freeCount = 0;

  public:
    void
    recordAllocation(const Allocation &alloc, uint32_t size)
    {
        /// check to see if an active allocation with the same offset already exists.
        assert(m_activeAllocations.find(alloc.offset) == m_activeAllocations.end() &&
               "allocator handed out same memory location twice!");

        AllocationRecord record
        {
            .offset = alloc.offset,
            .size = size,
            .nodeIndex = alloc.nodeIndex
        };
        // std::cout << "ALLOCATION recorded: \nOffset: " << record.offset << "\nSize: " << record.size
        //           << "\nNodeIndex: " << record.nodeIndex << "\n---------------------------------------\n";
        m_activeAllocations[alloc.offset] = record;
        m_totalAllocated += size;
        m_peakAllocated = std::max(m_peakAllocated, m_totalAllocated);
        m_allocationCount++;
    }

    void
    recordFree(const Allocation &alloc)
    {
        auto it = m_activeAllocations.find(alloc.offset);
        assert(it != m_activeAllocations.end() && "Freeing non-existent allocation!");
        assert(it->second.nodeIndex == alloc.nodeIndex && "Node index mismatch!");

        // std::cout << "FREE recorded: \nOffset: " << alloc.offset << "\nSize: " << it->second.size
        //           << "\nNodeIndex: " << alloc.nodeIndex << "\n---------------------------------------\n";
        m_totalAllocated -= it->second.size;
        m_activeAllocations.erase(it);
        m_freeCount++;
    }

    bool
    checkNoOverlaps() const
    {
        std::vector<std::pair<uint32_t, uint32_t>> ranges;
        for (const auto &pair : m_activeAllocations)
        {
            ranges.push_back({pair.second.offset, pair.second.offset + pair.second.size});
        }

        std::sort(ranges.begin(), ranges.end());

        for (size_t i = 1; i < ranges.size(); i++)
        {
            if (ranges[i].first < ranges[i - 1].second)
            {
                std::cerr << "Overlap detected: [" << ranges[i - 1].first << "-" << ranges[i - 1].second
                          << "] overlaps with [" << ranges[i].first << "-" << ranges[i].second << "]" << std::endl;
                return false;
            }
        }
        return true;
    }

    uint32_t getActiveAllocationCount() const { return static_cast<uint32_t>(m_activeAllocations.size()); }
    uint32_t getTotalAllocated() const { return m_totalAllocated; }
    uint32_t getPeakAllocated() const { return m_peakAllocated; }
    uint32_t getAllocationCount() const { return m_allocationCount; }
    uint32_t getFreeCount() const { return m_freeCount; }

    void
    printStats() const
    {
        std::cout << "=== Allocator Statistics ===" << std::endl;
        std::cout << "Active allocations: " << m_activeAllocations.size() << std::endl;
        std::cout << "Total allocated: " << m_totalAllocated << " bytes" << std::endl;
        std::cout << "Peak allocated: " << m_peakAllocated << " bytes" << std::endl;
        std::cout << "Total alloc calls: " << m_allocationCount << std::endl;
        std::cout << "Total free calls: " << m_freeCount << std::endl;
    }

    void
    reset()
    {
        m_activeAllocations.clear();
        m_totalAllocated = 0;
        m_allocationCount = 0;
        m_freeCount = 0;
    }
};

// ============================================================================
// Test Cases
// ============================================================================

bool
testBasicAllocation()
{
    std::cout << "\n[TEST] Basic Allocation" << std::endl;

    Allocator allocator(1024);
    AllocatorValidator validator;

    // Single allocation
    Allocation alloc = allocator.allocate(64);
    assert(alloc.offset != 0xffffffff && "Allocation failed!");
    validator.recordAllocation(alloc, 64);

    // Free it
    allocator.free(alloc);
    validator.recordFree(alloc);

    assert(validator.getActiveAllocationCount() == 0 && "Memory leak detected!");

    std::cout << "PASSED" << std::endl;
    return true;
}

bool
testMultipleAllocations()
{
    std::cout << "\n[TEST] Multiple Sequential Allocations" << std::endl;

    Allocator allocator(1024);
    AllocatorValidator validator;

    std::vector<Allocation> allocations;
    uint32_t sizes[] = {16, 32, 64, 128, 256, 512, 1024};

    for (uint32_t size : sizes)
    {
        Allocation alloc = allocator.allocate(size);
        assert(alloc.offset != 0xffffffff && "Allocation failed!");
        allocations.push_back(alloc);
        validator.recordAllocation(alloc, size);
    }

    assert(validator.checkNoOverlaps() && "Overlapping allocations detected!");

    // Free all
    for (size_t i = 0; i < allocations.size(); i++)
    {
        allocator.free(allocations[i]);
        validator.recordFree(allocations[i]);
    }

    assert(validator.getActiveAllocationCount() == 0 && "Memory leak detected!");
    validator.printStats();

    std::cout << "PASSED" << std::endl;
    return true;
}

bool
testFragmentation()
{
    std::cout << "\n[TEST] Fragmentation and Coalescing" << std::endl;

    Allocator allocator(2048);
    AllocatorValidator validator;

    // Allocate blocks
    std::vector<std::pair<Allocation, uint32_t>> allocations;
    for (int i = 0; i < 10; i++)
    {
        uint32_t size = 128;
        Allocation alloc = allocator.allocate(size);
        assert(alloc.offset != 0xffffffff && "Allocation failed!");
        allocations.push_back({alloc, size});
        validator.recordAllocation(alloc, size);
    }

    // Free every other block to create fragmentation
    for (size_t i = 1; i < allocations.size(); i += 2)
    {
        allocator.free(allocations[i].first);
        validator.recordFree(allocations[i].first);
    }

    std::cout << "After freeing every other block: " << validator.getActiveAllocationCount()
              << " active allocations" << std::endl;

    // Try to allocate a larger block (should use coalesced space)
    Allocation bigAlloc = allocator.allocate(256);
    if (bigAlloc.offset != 0xffffffff)
    {
        validator.recordAllocation(bigAlloc, 256);
        std::cout << "Successfully allocated 256 bytes in fragmented space" << std::endl;
        allocator.free(bigAlloc);
        validator.recordFree(bigAlloc);
    }

    // Free remaining
    for (size_t i = 0; i < allocations.size(); i += 2)
    {
        allocator.free(allocations[i].first);
        validator.recordFree(allocations[i].first);
    }

    assert(validator.getActiveAllocationCount() == 0 && "Memory leak detected!");
    validator.printStats();

    std::cout << "PASSED" << std::endl;
    return true;
}

bool
testRandomAllocFree()
{
    std::cout << "\n[TEST] Random Allocation/Free Pattern" << std::endl;

    Allocator allocator(4096);
    AllocatorValidator validator;

    std::random_device rd;
    std::mt19937 gen(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<> sizeDist(8, 512);
    std::uniform_real_distribution<> actionDist(0.0, 1.0);

    std::vector<std::pair<Allocation, uint32_t>> activeAllocs;
    const int iterations = 1000;
    std::cout << "----------------------------------------------------------------\n";
    for (int i = 0; i < iterations; i++)
    {
        double action = actionDist(gen);

        // 60% allocate, 40% free
        if (action < 0.6 || activeAllocs.empty())
        {
            uint32_t size = sizeDist(gen);
            Allocation alloc = allocator.allocate(size);

            if (alloc.offset != 0xffffffff)
            {
                activeAllocs.push_back({alloc, size});
                validator.recordAllocation(alloc, size);
            }
        }
        else if (!activeAllocs.empty())
        {
            std::uniform_int_distribution<> indexDist(0, static_cast<int>(activeAllocs.size()) - 1);
            size_t idx = indexDist(gen);

            if (activeAllocs[idx].first.offset == 57) {
                int x = 0;
            }

            allocator.free(activeAllocs[idx].first);
            validator.recordFree(activeAllocs[idx].first);

            activeAllocs.erase(activeAllocs.begin() + idx);
        }

        if (i % 100 == 0)
        {
            assert(validator.checkNoOverlaps() && "Overlapping allocations detected!");
        }
    }

    std::cout << "Active allocations remaining: " << activeAllocs.size() << std::endl;
    assert(validator.checkNoOverlaps() && "Overlapping allocations detected!");

    // Cleanup
    for (auto &pair : activeAllocs)
    {
        allocator.free(pair.first);
        validator.recordFree(pair.first);
    }

    assert(validator.getActiveAllocationCount() == 0 && "Memory leak detected!");
    validator.printStats();

    std::cout << "PASSED" << std::endl;
    return true;
}

bool
testBoundaryConditions()
{
    std::cout << "\n[TEST] Boundary Conditions" << std::endl;

    Allocator allocator(1024);
    AllocatorValidator validator;

    // Minimum size allocation
    Allocation minAlloc = allocator.allocate(8);
    assert(minAlloc.offset != 0xffffffff && "Min size allocation failed!");
    validator.recordAllocation(minAlloc, 8);

    // Very small size (should round up to minimum)
    Allocation tinyAlloc = allocator.allocate(1);
    assert(tinyAlloc.offset != 0xffffffff && "Tiny allocation failed!");
    validator.recordAllocation(tinyAlloc, 1);

    // Power of 2 sizes
    uint32_t powerSizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048};
    std::vector<std::pair<Allocation, uint32_t>> powerAllocs;

    for (uint32_t size : powerSizes)
    {
        Allocation alloc = allocator.allocate(size);
        if (alloc.offset != 0xffffffff)
        {
            powerAllocs.push_back({alloc, size});
            validator.recordAllocation(alloc, size);
        }
    }
    
    assert(validator.checkNoOverlaps() && "Overlapping allocations detected!");

    // Cleanup
    allocator.free(minAlloc);
    validator.recordFree(minAlloc);
    allocator.free(tinyAlloc);
    validator.recordFree(tinyAlloc);

    for (auto &pair : powerAllocs)
    {
        allocator.free(pair.first);
        validator.recordFree(pair.first);
    }

    assert(validator.getActiveAllocationCount() == 0 && "Memory leak detected!");
    validator.printStats();

    std::cout << "PASSED" << std::endl;
    return true;
}

bool
testStressAllocation()
{
    std::cout << "\n[TEST] Stress Test - Many Small Allocations" << std::endl;

    Allocator allocator(8192);
    AllocatorValidator validator;

    std::vector<std::pair<Allocation, uint32_t>> allocations;
    const int count = 500;

    // Allocate many small blocks
    for (int i = 0; i < count; i++)
    {
        uint32_t size = 16;
        Allocation alloc = allocator.allocate(size);

        if (alloc.offset != 0xffffffff)
        {
            allocations.push_back({alloc, size});
            validator.recordAllocation(alloc, size);
        }
        else
        {
            std::cout << "Failed to allocate after " << i << " allocations" << std::endl;
            break;
        }
    }

    std::cout << "Successfully allocated " << allocations.size() << " blocks" << std::endl;
    assert(validator.checkNoOverlaps() && "Overlapping allocations detected!");

    // Free all
    for (auto &pair : allocations)
    {
        allocator.free(pair.first);
        validator.recordFree(pair.first);
    }

    assert(validator.getActiveAllocationCount() == 0 && "Memory leak detected!");
    validator.printStats();

    std::cout << "PASSED" << std::endl;
    return true;
}

bool
testAlternatingPattern()
{
    std::cout << "\n[TEST] Alternating Alloc/Free Pattern" << std::endl;

    Allocator allocator(2048);
    AllocatorValidator validator;

    const int iterations = 100;

    for (int i = 0; i < iterations; i++)
    {
        // Allocate
        Allocation alloc1 = allocator.allocate(64);
        assert(alloc1.offset != 0xffffffff && "Allocation 1 failed!");
        validator.recordAllocation(alloc1, 64);

        Allocation alloc2 = allocator.allocate(128);
        assert(alloc2.offset != 0xffffffff && "Allocation 2 failed!");
        validator.recordAllocation(alloc2, 128);

        // Free immediately
        allocator.free(alloc1);
        validator.recordFree(alloc1);

        allocator.free(alloc2);
        validator.recordFree(alloc2);
    }

    assert(validator.getActiveAllocationCount() == 0 && "Memory leak detected!");
    validator.printStats();

    std::cout << "PASSED" << std::endl;
    return true;
}

bool
testReuseAfterFree()
{
    std::cout << "\n[TEST] Memory Reuse After Free" << std::endl;

    Allocator allocator(1024);
    AllocatorValidator validator;

    // Allocate and free
    Allocation alloc1 = allocator.allocate(256);
    uint32_t offset1 = alloc1.offset;
    validator.recordAllocation(alloc1, 256);

    allocator.free(alloc1);
    validator.recordFree(alloc1);

    // Allocate same size again - should ideally reuse same space
    Allocation alloc2 = allocator.allocate(256);
    validator.recordAllocation(alloc2, 256);

    std::cout << "First allocation offset: " << offset1 << std::endl;
    std::cout << "Second allocation offset: " << alloc2.offset << std::endl;

    if (offset1 == alloc2.offset)
    {
        std::cout << "Memory successfully reused!" << std::endl;
    }

    allocator.free(alloc2);
    validator.recordFree(alloc2);

    assert(validator.getActiveAllocationCount() == 0 && "Memory leak detected!");

    std::cout << "PASSED" << std::endl;
    return true;
}

// ============================================================================
// Test Runner
// ============================================================================

void
runAllTests()
{
    std::cout << "========================================" << std::endl;
    std::cout << "TLSF Allocator Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    int passed = 0;
    int total = 0;

    auto runTest = [&](bool (*testFunc)(), const char *name)
    {
        total++;
        try
        {
            if (testFunc())
            {
                passed++;
            }
            else
            {
                std::cerr << "FAILED: " << name << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "EXCEPTION in " << name << ": " << e.what() << std::endl;
        }
    };

    runTest(testBasicAllocation, "Basic Allocation");
    runTest(testMultipleAllocations, "Multiple Allocations");
    runTest(testFragmentation, "Fragmentation");
    runTest(testRandomAllocFree, "Random Alloc/Free");
    runTest(testBoundaryConditions, "Boundary Conditions");
    runTest(testStressAllocation, "Stress Test");
    runTest(testAlternatingPattern, "Alternating Pattern");
    runTest(testReuseAfterFree, "Memory Reuse");

    std::cout << "\n========================================" << std::endl;
    std::cout << "Tests Passed: " << passed << "/" << total << std::endl;
    std::cout << "========================================" << std::endl;
}

} // namespace Testing

} // namespace Tlsf

#endif


