#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstddef>
#include "memory_allocator.h"

using namespace std;

// ==================== MEMORY MANAGER IMPLEMENTATION ====================

// Constructor
MemoryManager::MemoryManager(size_t size) 
    : total_memory(size), strategy(AllocationStrategy::FIRST_FIT), next_block_id(1),
      allocation_attempts(0), allocation_successes(0), allocation_failures(0) {
    head = new MemoryBlock(0, size, false, -1);
    cout << "Memory initialized: " << size << " bytes\n";
}

// Destructor
MemoryManager::~MemoryManager() {
    MemoryBlock* current = head;
    while (current != nullptr) {
        MemoryBlock* next = current->next;
        delete current;
        current = next;
    }
}

// Find block using First Fit
MemoryBlock* MemoryManager::findBlockFirstFit(size_t size) {
    MemoryBlock* current = head;
    while (current != nullptr) {
        if (!current->is_allocated && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

// Find block using Best Fit
MemoryBlock* MemoryManager::findBlockBestFit(size_t size) {
    MemoryBlock* current = head;
    MemoryBlock* best = nullptr;
    size_t min_diff = total_memory + 1;
    
    while (current != nullptr) {
        if (!current->is_allocated && current->size >= size) {
            size_t diff = current->size - size;
            if (diff < min_diff) {
                min_diff = diff;
                best = current;
            }
        }
        current = current->next;
    }
    return best;
}

// Find block using Worst Fit
MemoryBlock* MemoryManager::findBlockWorstFit(size_t size) {
    MemoryBlock* current = head;
    MemoryBlock* worst = nullptr;
    size_t max_size = 0;
    
    while (current != nullptr) {
        if (!current->is_allocated && current->size >= size) {
            if (current->size > max_size) {
                max_size = current->size;
                worst = current;
            }
        }
        current = current->next;
    }
    return worst;
}

// Split block if needed
void MemoryManager::splitBlock(MemoryBlock* block, size_t size) {
    if (block->size > size) {
        MemoryBlock* newBlock = new MemoryBlock(
            block->start_address + size,
            block->size - size,
            false,
            -1
        );
        
        newBlock->next = block->next;
        newBlock->prev = block;
        
        if (block->next != nullptr) {
            block->next->prev = newBlock;
        }
        
        block->next = newBlock;
        block->size = size;
    }
}

// Coalesce adjacent free blocks
void MemoryManager::coalesceBlocks() {
    MemoryBlock* current = head;
    
    while (current != nullptr && current->next != nullptr) {
        if (!current->is_allocated && !current->next->is_allocated) {
            MemoryBlock* nextBlock = current->next;
            current->size += nextBlock->size;
            current->next = nextBlock->next;
            
            if (nextBlock->next != nullptr) {
                nextBlock->next->prev = current;
            }
            
            delete nextBlock;
            continue;
        }
        current = current->next;
    }
}

// Set allocation strategy
void MemoryManager::setStrategy(AllocationStrategy s) {
    strategy = s;
    cout << "Allocation strategy set to: ";
    switch(s) {
        case AllocationStrategy::FIRST_FIT:
            cout << "First Fit\n";
            break;
        case AllocationStrategy::BEST_FIT:
            cout << "Best Fit\n";
            break;
        case AllocationStrategy::WORST_FIT:
            cout << "Worst Fit\n";
            break;
    }
}

// Allocate memory
int MemoryManager::allocate(size_t size) {
    allocation_attempts++;
    
    if (size == 0) {
        allocation_failures++;
        cout << "Error: Cannot allocate 0 bytes\n";
        return -1;
    }
    
    MemoryBlock* block = nullptr;
    
    switch(strategy) {
        case AllocationStrategy::FIRST_FIT:
            block = findBlockFirstFit(size);
            break;
        case AllocationStrategy::BEST_FIT:
            block = findBlockBestFit(size);
            break;
        case AllocationStrategy::WORST_FIT:
            block = findBlockWorstFit(size);
            break;
    }
    
    if (block == nullptr) {
        allocation_failures++;
        cout << "Error: Not enough memory to allocate " << size << " bytes\n";
        return -1;
    }
    
    splitBlock(block, size);
    block->is_allocated = true;
    block->block_id = next_block_id;
    
    allocation_successes++;
    
    cout << "Allocated block id=" << next_block_id 
         << " at address=0x" << hex << setw(4) 
         << setfill('0') << block->start_address 
         << dec << " (" << size << " bytes)\n";
    
    return next_block_id++;
}

// Free memory
bool MemoryManager::deallocate(int block_id) {
    MemoryBlock* current = head;
    
    while (current != nullptr) {
        if (current->is_allocated && current->block_id == block_id) {
            current->is_allocated = false;
            current->block_id = -1;
            
            cout << "Block " << block_id << " freed";
            coalesceBlocks();
            cout << " and merged\n";
            return true;
        }
        current = current->next;
    }
    
    cout << "Error: Block " << block_id << " not found\n";
    return false;
}

// Display memory layout
void MemoryManager::displayMemory() const {
    cout << "\n=== Memory Layout ===\n";
    MemoryBlock* current = head;
    
    while (current != nullptr) {
        cout << "[0x" << hex << setw(4) << setfill('0') 
             << current->start_address << " - 0x" 
             << setw(4) << setfill('0')
             << (current->start_address + current->size - 1) << "] ";
        
        if (current->is_allocated) {
            cout << "USED (id=" << dec << current->block_id << ")\n";
        } else {
            cout << "FREE\n";
        }
        
        current = current->next;
    }
    cout << dec << endl;
}

// Get used memory
size_t MemoryManager::getUsedMemory() const {
    size_t used = 0;
    MemoryBlock* current = head;
    while (current != nullptr) {
        if (current->is_allocated) {
            used += current->size;
        }
        current = current->next;
    }
    return used;
}

// Get free memory
size_t MemoryManager::getFreeMemory() const {
    return total_memory - getUsedMemory();
}

// Get external fragmentation
double MemoryManager::getExternalFragmentation() const {
    size_t largest_free = 0;
    size_t total_free = 0;
    MemoryBlock* current = head;
    
    while (current != nullptr) {
        if (!current->is_allocated) {
            total_free += current->size;
            if (current->size > largest_free) {
                largest_free = current->size;
            }
        }
        current = current->next;
    }
    
    if (total_free == 0) return 0.0;
    return ((double)(total_free - largest_free) / total_free) * 100.0;
}

// Count free blocks
int MemoryManager::countFreeBlocks() const {
    int count = 0;
    MemoryBlock* current = head;
    while (current != nullptr) {
        if (!current->is_allocated) {
            count++;
        }
        current = current->next;
    }
    return count;
}

// Get allocation success rate
double MemoryManager::getAllocationSuccessRate() const {
    if (allocation_attempts == 0) return 0.0;
    return ((double)allocation_successes / allocation_attempts) * 100.0;
}

// Get allocation attempts
int MemoryManager::getAllocationAttempts() const { 
    return allocation_attempts; 
}

// Get allocation successes
int MemoryManager::getAllocationSuccesses() const { 
    return allocation_successes; 
}

// Get allocation failures
int MemoryManager::getAllocationFailures() const { 
    return allocation_failures; 
}

// Display statistics
void MemoryManager::displayStats() const {
    cout << "\n=== Memory Statistics ===\n";
    cout << "Total memory: " << total_memory << " bytes\n";
    cout << "Used memory: " << getUsedMemory() << " bytes\n";
    cout << "Free memory: " << getFreeMemory() << " bytes\n";
    cout << "Free blocks: " << countFreeBlocks() << "\n";
    cout << "External fragmentation: " << fixed 
         << setprecision(2) << getExternalFragmentation() << "%\n";
    cout << "Internal fragmentation: 0 bytes (exact allocation)\n";
    
    cout << "\nAllocation Statistics:\n";
    cout << "Total attempts: " << getAllocationAttempts() << "\n";
    cout << "Successful: " << getAllocationSuccesses() << "\n";
    cout << "Failed: " << getAllocationFailures() << "\n";
    cout << "Success rate: " << fixed << setprecision(2) 
         << getAllocationSuccessRate() << "%\n";
    cout << endl;
}