#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
#include <vector>
#include <map>
#include <cmath>
#include <iomanip>
#include <sstream>
#include "buddy_allocator.h"

using namespace std;

// ==================== BUDDY SYSTEM DATA STRUCTURES ====================
    
// Helper: Check if size is power of 2
bool BuddyAllocator::isPowerOfTwo(size_t n) {
    return n > 0 && (n & (n - 1)) == 0;
}

// Helper: Calculate order (log2 relative to min_block_size)
int  BuddyAllocator::getOrder(size_t size) {
    if (size <= min_block_size) return 0;
    
    int order = 0;
    size_t temp = size / min_block_size;
    while (temp > 1) {
        temp /= 2;
        order++;
    }
    return order;
}

// Helper: Get size for given order
size_t  BuddyAllocator::getBlockSize(int order) const{
    return min_block_size * (1 << order);
}

// Helper: Calculate buddy address
size_t  BuddyAllocator::getBuddyAddress(size_t address, size_t size) {
    return address ^ size;
}

// Helper: Round up to next power of 2
size_t  BuddyAllocator::nextPowerOfTwo(size_t size) {
    if (size == 0) return min_block_size;
    if (size <= min_block_size) return min_block_size;
    
    size_t power = min_block_size;
    while (power < size) {
        power *= 2;
    }
    return power;
}

// Split a block into two buddies
bool  BuddyAllocator::splitBlock(int order) {
    if (order >= max_order) {
        return false;  // Cannot split beyond max order
    }
    
    // Get a block from higher order
    if (free_lists[order + 1] == nullptr) {
        // Need to split higher order first
        if (!splitBlock(order + 1)) {
            return false;
        }
    }
    
    if (free_lists[order + 1] == nullptr) {
        return false;  // Still no block available
    }
    
    // Remove block from higher order
    BuddyBlock* block = free_lists[order + 1];
    free_lists[order + 1] = block->next;
    
    // Calculate size for this order
    size_t new_size = getBlockSize(order);
    
    // Create two buddy blocks
    BuddyBlock* buddy1 = new BuddyBlock(block->address, new_size);
    BuddyBlock* buddy2 = new BuddyBlock(block->address + new_size, new_size);
    
    // Add both to current order's free list
    buddy1->next = free_lists[order];
    free_lists[order] = buddy1;
    
    buddy2->next = free_lists[order];
    free_lists[order] = buddy2;
    
    splits++;
    delete block;
    
    return true;
}

// Find and remove a block from free list
BuddyBlock*  BuddyAllocator::removeFromFreeList(int order, size_t address) {
    if (free_lists[order] == nullptr) return nullptr;
    
    // Check if first block matches
    if (free_lists[order]->address == address) {
        BuddyBlock* block = free_lists[order];
        free_lists[order] = block->next;
        return block;
    }
    
    // Search in list
    BuddyBlock* current = free_lists[order];
    while (current->next != nullptr) {
        if (current->next->address == address) {
            BuddyBlock* block = current->next;
            current->next = block->next;
            return block;
        }
        current = current->next;
    }
    
    return nullptr;
}

// Try to merge block with its buddy
bool  BuddyAllocator::mergeBlocks(size_t address, int order) {
    if (order >= max_order) {
        // Already at max order, cannot merge further
        return false;
    }
    
    size_t block_size = getBlockSize(order);
    
    // Calculate buddy address
    size_t buddy_addr = getBuddyAddress(address, block_size);
    
    // Try to find buddy in free list
    BuddyBlock* buddy = removeFromFreeList(order, buddy_addr);
    
    if (buddy == nullptr) {
        // Buddy not free, cannot merge
        return false;
    }
    
    // Buddy found! Remove current block from free list
    removeFromFreeList(order, address);
    
    // Merge
    size_t merged_addr = (address < buddy_addr) ? address : buddy_addr;
    size_t merged_size = block_size * 2;
    
    // Add merged block to higher order
    BuddyBlock* merged = new BuddyBlock(merged_addr, merged_size);
    merged->next = free_lists[order + 1];
    free_lists[order + 1] = merged;
    
    merges++;
    delete buddy;
    
    // Try to merge recursively
    mergeBlocks(merged_addr, order + 1);
    
    return true;
}

// Constructor
    BuddyAllocator::BuddyAllocator(size_t memory_size, size_t min_size) 
    : total_memory(memory_size), min_block_size(min_size),
        next_block_id(1),
        total_allocations(0), total_deallocations(0),
        successful_allocations(0), failed_allocations(0),
        splits(0), merges(0), total_internal_fragmentation(0) {
    
    // Validate memory size is power of 2
    if (!isPowerOfTwo(total_memory)) {
        cout << "Error: Memory size must be power of 2\n";
        total_memory = 1024;  // Default
    }
    
    // Validate min block size is power of 2
    if (!isPowerOfTwo(min_block_size)) {
        cout << "Error: Min block size must be power of 2\n";
        min_block_size = 16;  // Default
    }
    
    // Calculate max order
    max_order = 0;
    size_t temp = total_memory / min_block_size;
    while (temp > 1) {
        temp /= 2;
        max_order++;
    }
    
    // Initialize free lists
    free_lists.resize(max_order + 1, nullptr);
    
    // Add entire memory as one block at max order
    BuddyBlock* initial = new BuddyBlock(0, total_memory);
    free_lists[max_order] = initial;
    
    cout << "Buddy Allocator initialized:\n";
    cout << "  Total memory: " << total_memory << " bytes\n";
    cout << "  Min block size: " << min_block_size << " bytes\n";
    cout << "  Max order: " << max_order << "\n";
    cout << "  Orders: 0 (" << min_block_size << " bytes) to " 
            << max_order << " (" << total_memory << " bytes)\n";
}

// Destructor
    BuddyAllocator::~BuddyAllocator() {
    for (int i = 0; i <= max_order; i++) {
        BuddyBlock* current = free_lists[i];
        while (current != nullptr) {
            BuddyBlock* next = current->next;
            delete current;
            current = next;
        }
    }
}

// Allocate memory - returns block_id
int  BuddyAllocator::allocate(size_t requested_size) {
    total_allocations++;
    
    if (requested_size == 0) {
        cout << "Error: Cannot allocate 0 bytes\n";
        failed_allocations++;
        return -1;
    }
    
    if (requested_size > total_memory) {
        cout << "Error: Requested size exceeds total memory\n";
        failed_allocations++;
        return -1;
    }
    
    // Round up to next power of 2
    size_t actual_size = nextPowerOfTwo(requested_size);
    int order = getOrder(actual_size);
    
    cout << "\nAllocation request #" << total_allocations << ":\n";
    cout << "  Requested: " << requested_size << " bytes\n";
    cout << "  Rounded to: " << actual_size << " bytes (order " << order << ")\n";
    
    // Ensure we have a block of this size
    if (free_lists[order] == nullptr) {
        cout << "  No free block at order " << order << ", splitting larger blocks...\n";
        if (!splitBlock(order)) {
            cout << "  ERROR: Out of memory (cannot split)\n";
            failed_allocations++;
            return -1;
        }
    }
    
    // Check if split succeeded
    if (free_lists[order] == nullptr) {
        cout << "  ERROR: Out of memory\n";
        failed_allocations++;
        return -1;
    }
    
    // Allocate block
    BuddyBlock* block = free_lists[order];
    free_lists[order] = block->next;
    block->is_free = false;
    
    int block_id = next_block_id++;
    block->block_id = block_id;
    
    successful_allocations++;
    
    // Calculate internal fragmentation
    size_t internal_frag = actual_size - requested_size;
    total_internal_fragmentation += internal_frag;
    
    // Store allocation record
    AllocationRecord record;
    record.address = block->address;
    record.requested_size = requested_size;
    record.actual_size = actual_size;
    record.order = order;
    allocated_blocks[block_id] = record;
    
    cout << "  SUCCESS: Allocated block_id=" << block_id;
    cout << " at address 0x" << hex << block->address << dec;
    cout << " (size=" << actual_size << " bytes)\n";
    
    if (internal_frag > 0) {
        cout << "  Internal fragmentation: " << internal_frag << " bytes";
        double frag_percent = (double)internal_frag / actual_size * 100.0;
        cout << " (" << fixed << setprecision(2) << frag_percent << "%)\n";
    }
    
    delete block;  // We don't need the block structure anymore
    
    return block_id;
}

// Deallocate memory by block_id
bool  BuddyAllocator::deallocate(int block_id) {
    auto it = allocated_blocks.find(block_id);
    if (it == allocated_blocks.end()) {
        cout << "\nError: Invalid block_id " << block_id << "\n";
        return false;
    }
    
    AllocationRecord& record = it->second;
    
    cout << "\nDeallocation:\n";
    cout << "  Block ID: " << block_id << "\n";
    cout << "  Address: 0x" << hex << record.address << dec << "\n";
    cout << "  Size: " << record.actual_size << " bytes (order " << record.order << ")\n";
    
    // Add block back to free list
    BuddyBlock* block = new BuddyBlock(record.address, record.actual_size);
    block->next = free_lists[record.order];
    free_lists[record.order] = block;
    
    total_deallocations++;
    
    // Subtract internal fragmentation
    size_t internal_frag = record.actual_size - record.requested_size;
    total_internal_fragmentation -= internal_frag;
    
    // Remove from allocated blocks
    allocated_blocks.erase(it);
    
    // Try to merge with buddy
    cout << "  Attempting to merge with buddy...\n";
    int merges_before = merges;
    mergeBlocks(record.address, record.order);
    int merges_done = merges - merges_before;
    if (merges_done > 0) {
        cout << "  Performed " << merges_done << " merge(s)\n";
    } else {
        cout << "  No merge possible (buddy not free)\n";
    }
    
    return true;
}

// Display free lists
void  BuddyAllocator::displayFreeLists() const {
    cout << "\n=== FREE LISTS ===\n";
    cout << "Format: Order (block_size): [address1] [address2] ...\n\n";
    
    bool has_free = false;
    for (int i = 0; i <= max_order; i++) {
        size_t block_size = getBlockSize(i);
        
        BuddyBlock* current = free_lists[i];
        int count = 0;
        while (current != nullptr) {
            count++;
            current = current->next;
        }
        
        cout << "Order " << setw(2) << i << " (" << setw(6) << block_size << " bytes): ";
        
        current = free_lists[i];
        if (current == nullptr) {
            cout << "empty";
        } else {
            has_free = true;
            while (current != nullptr) {
                cout << "[0x" << hex << setfill('0') << setw(4) << current->address << dec << "] ";
                current = current->next;
            }
            cout << "(" << count << " block" << (count > 1 ? "s" : "") << ")";
        }
        cout << "\n";
    }
    
    if (!has_free) {
        cout << "\n*** All memory is allocated ***\n";
    }
}

// Display allocated blocks
void  BuddyAllocator::displayAllocatedBlocks() const {
    cout << "\n=== ALLOCATED BLOCKS ===\n";
    
    if (allocated_blocks.empty()) {
        cout << "No blocks currently allocated\n";
        return;
    }
    
    cout << "Format: block_id | address | requested -> actual | internal_frag\n\n";
    
    for (const auto& pair : allocated_blocks) {
        const AllocationRecord& rec = pair.second;
        size_t internal_frag = rec.actual_size - rec.requested_size;
        
        cout << "Block " << setw(3) << pair.first;
        cout << " | 0x" << hex << setfill('0') << setw(4) << rec.address << dec;
        cout << " | " << setw(5) << rec.requested_size << " -> " << setw(5) << rec.actual_size;
        cout << " | " << setw(4) << internal_frag << " bytes";
        
        if (internal_frag > 0) {
            double percent = (double)internal_frag / rec.actual_size * 100.0;
            cout << " (" << fixed << setprecision(1) << percent << "%)";
        }
        cout << "\n";
    }
}

// Display statistics
void  BuddyAllocator::displayStats() const {
    cout << "\n=== BUDDY ALLOCATOR STATISTICS ===\n";
    
    // Configuration
    cout << "\nConfiguration:\n";
    cout << "  Total memory: " << total_memory << " bytes\n";
    cout << "  Min block size: " << min_block_size << " bytes\n";
    cout << "  Max order: " << max_order << "\n";
    
    // Allocation statistics
    cout << "\nAllocation Statistics:\n";
    cout << "  Total allocation attempts: " << total_allocations << "\n";
    cout << "  Successful allocations: " << successful_allocations << "\n";
    cout << "  Failed allocations: " << failed_allocations << "\n";
    
    if (total_allocations > 0) {
        double success_rate = (double)successful_allocations / total_allocations * 100.0;
        cout << "  Success rate: " << fixed << setprecision(2) << success_rate << "%\n";
    }
    
    cout << "  Total deallocations: " << total_deallocations << "\n";
    cout << "  Currently allocated blocks: " << allocated_blocks.size() << "\n";
    
    // Split/Merge statistics
    cout << "\nSplit/Merge Operations:\n";
    cout << "  Block splits: " << splits << "\n";
    cout << "  Block merges: " << merges << "\n";
    
    // Memory utilization
    cout << "\nMemory Utilization:\n";
    size_t free_memory = 0;
    for (int i = 0; i <= max_order; i++) {
        BuddyBlock* current = free_lists[i];
        while (current != nullptr) {
            free_memory += current->size;
            current = current->next;
        }
    }
    
    size_t used_memory = total_memory - free_memory;
    double utilization = (double)used_memory / total_memory * 100.0;
    
    cout << "  Total memory: " << total_memory << " bytes\n";
    cout << "  Used memory: " << used_memory << " bytes\n";
    cout << "  Free memory: " << free_memory << " bytes\n";
    cout << "  Utilization: " << fixed << setprecision(2) << utilization << "%\n";
    
    // Fragmentation
    cout << "\nFragmentation:\n";
    cout << "  Total internal fragmentation: " << total_internal_fragmentation << " bytes\n";
    
    if (used_memory > 0) {
        double internal_frag_percent = (double)total_internal_fragmentation / used_memory * 100.0;
        cout << "  Internal fragmentation ratio: " << fixed << setprecision(2) 
                << internal_frag_percent << "% of used memory\n";
    }
    
    // External fragmentation (number of free blocks at different orders)
    int total_free_blocks = 0;
    for (int i = 0; i < max_order; i++) {  // Don't count max order
        BuddyBlock* current = free_lists[i];
        while (current != nullptr) {
            total_free_blocks++;
            current = current->next;
        }
    }
    
    if (total_free_blocks > 0) {
        cout << "  External fragmentation: " << total_free_blocks 
                << " free block(s) smaller than max size\n";
    } else if (free_memory == 0) {
        cout << "  External fragmentation: N/A (no free memory)\n";
    } else {
        cout << "  External fragmentation: None (all free memory in one block)\n";
    }
}