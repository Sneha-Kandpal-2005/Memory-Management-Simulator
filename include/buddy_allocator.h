#ifndef BUDDY_ALLOCATOR_H
#define BUDDY_ALLOCATOR_H

#include <iostream>
#include <vector>
#include <map>
#include <cstddef>

using namespace std;

// ==================== BUDDY BLOCK STRUCTURE ====================

struct BuddyBlock {
    size_t address;
    size_t size;
    bool is_free;
    int block_id;
    BuddyBlock* next;
    
    BuddyBlock(size_t addr, size_t sz, int id = -1) 
        : address(addr), size(sz), is_free(true), block_id(id), next(nullptr) {}
};

// ==================== ALLOCATION RECORD STRUCTURE ====================

struct AllocationRecord {
    size_t address;
    size_t requested_size;
    size_t actual_size;
    int order;
};

// ==================== BUDDY ALLOCATOR CLASS ====================

class BuddyAllocator {
private:
    size_t total_memory;
    size_t min_block_size;
    int max_order;
    
    vector<BuddyBlock*> free_lists;
    map<int, AllocationRecord> allocated_blocks;
    
    int next_block_id;
    
    // Statistics
    int total_allocations;
    int total_deallocations;
    int successful_allocations;
    int failed_allocations;
    int splits;
    int merges;
    size_t total_internal_fragmentation;
    
    // Helper functions
    bool isPowerOfTwo(size_t n);
    int getOrder(size_t size);
    size_t getBlockSize(int order) const;
    size_t getBuddyAddress(size_t address, size_t size);
    size_t nextPowerOfTwo(size_t size);
    bool splitBlock(int order);
    BuddyBlock* removeFromFreeList(int order, size_t address);
    bool mergeBlocks(size_t address, int order);
    
public:
    // Constructor & Destructor
    BuddyAllocator(size_t memory_size, size_t min_size = 16);
    ~BuddyAllocator();
    
    // Main operations
    int allocate(size_t requested_size);
    bool deallocate(int block_id);
    
    // Display functions
    void displayFreeLists() const;
    void displayAllocatedBlocks() const;
    void displayStats() const;
    
    size_t getTotalMemory() const { return total_memory; }
    size_t getMinBlockSize() const { return min_block_size; }
    int getMaxOrder() const { return max_order; }
    int getSuccessfulAllocations() const { return successful_allocations; }
    int getFailedAllocations() const { return failed_allocations; }
    int getSplits() const { return splits; }
    int getMerges() const { return merges; }
    size_t getInternalFragmentation() const { return total_internal_fragmentation; }
};

#endif // BUDDY_ALLOCATOR_H