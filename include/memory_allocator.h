#ifndef MEMORY_ALLOCATOR_H
#define MEMORY_ALLOCATOR_H

#include <iostream>
#include <string>
#include <cstddef>

// ==================== MEMORY BLOCK STRUCTURE ====================

struct MemoryBlock {
    size_t start_address;
    size_t size;
    bool is_allocated;
    int block_id;
    MemoryBlock* next;
    MemoryBlock* prev;
    
    MemoryBlock(size_t addr, size_t sz, bool alloc = false, int id = -1)
        : start_address(addr), size(sz), is_allocated(alloc), 
          block_id(id), next(nullptr), prev(nullptr) {}
};

// ==================== ALLOCATION STRATEGY ENUM ====================

enum class AllocationStrategy {
    FIRST_FIT,
    BEST_FIT,
    WORST_FIT
};

// ==================== MEMORY MANAGER CLASS ====================

class MemoryManager {
private:
    size_t total_memory;
    MemoryBlock* head;
    AllocationStrategy strategy;
    int next_block_id;
    
    // Allocation tracking
    int allocation_attempts;
    int allocation_successes;
    int allocation_failures;
    
    // Helper functions
    MemoryBlock* findBlockFirstFit(size_t size);
    MemoryBlock* findBlockBestFit(size_t size);
    MemoryBlock* findBlockWorstFit(size_t size);
    void splitBlock(MemoryBlock* block, size_t size);
    void coalesceBlocks();
    
public:
    // Constructor & Destructor
    MemoryManager(size_t size);
    ~MemoryManager();
    
    // Main operations
    void setStrategy(AllocationStrategy s);
    int allocate(size_t size);
    bool deallocate(int block_id);
    
    // Display functions
    void displayMemory() const;
    void displayStats() const;
    
    // Getters
    size_t getUsedMemory() const;
    size_t getFreeMemory() const;
    double getExternalFragmentation() const;
    int countFreeBlocks() const;
    double getAllocationSuccessRate() const;
    int getAllocationAttempts() const;
    int getAllocationSuccesses() const;
    int getAllocationFailures() const;
};

#endif // MEMORY_ALLOCATOR_H