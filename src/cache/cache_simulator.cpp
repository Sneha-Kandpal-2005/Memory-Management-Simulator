#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
#include <vector>
#include <iomanip>
#include <sstream>
#include <string>
#include <algorithm>
#include <cmath>
#include "cache_simulator.h"

using namespace std;

// ==================== CACHE CLASS IMPLEMENTATION ====================
    
// Helper: Extract set index from address
int Cache::getSetIndex(size_t address) {
    size_t block_number = address / block_size;
    return block_number % num_sets;
}
    
// Helper: Extract tag from address
size_t Cache::getTag(size_t address) {
    size_t block_number = address / block_size;
    return block_number / num_sets;
}
    
// Helper: Find victim in a set
int Cache::findVictimInSet(int set_index) {
    // First, check for invalid (empty) slot
    for (int way = 0; way < ways; way++) {
        if (!cache[set_index][way].valid) {
            return way;
        }
    }
        
    // All ways full, find victim based on policy
    if (replacement_policy == ReplacementPolicy::FIFO) {
        return findFIFOVictimInSet(set_index);
    } else {
        return findLRUVictimInSet(set_index);
    }
}
    
// FIFO: Find oldest entry in set
int Cache::findFIFOVictimInSet(int set_index) {
    int oldest_way = 0;
    int oldest_order = cache[set_index][0].insertion_order;
        
    for (int way = 1; way < ways; way++) {
        if (cache[set_index][way].insertion_order < oldest_order) {
            oldest_order = cache[set_index][way].insertion_order;
            oldest_way = way;
        }
    }
        
    return oldest_way;
}
    
// LRU: Find least recently used entry in set
int Cache::findLRUVictimInSet(int set_index) {
    int lru_way = 0;
    int lru_time = cache[set_index][0].last_access_time;
        
    for (int way = 1; way < ways; way++) {
        if (cache[set_index][way].last_access_time < lru_time) {
            lru_time = cache[set_index][way].last_access_time;
            lru_way = way;
        }
    }
        
    return lru_way;
}
    

// Constructor
Cache::Cache(string cache_name, int total_lines, int blk_size, 
             AssociativityType assoc, ReplacementPolicy repl_pol, WritePolicy wr_pol) 
    : name(cache_name), capacity(total_lines), block_size(blk_size),
      associativity(assoc), replacement_policy(repl_pol), write_policy(wr_pol),
      next_insertion_order(0), access_counter(0), 
      hits(0), misses(0), writes(0), write_hits(0), write_misses(0), writebacks(0) {
    
    // Calculate number of sets and ways based on associativity
    switch(associativity) {
        case AssociativityType::DIRECT_MAPPED:
            ways = 1;
            num_sets = capacity;
            break;
        case AssociativityType::TWO_WAY:
            ways = 2;
            num_sets = capacity / 2;
            break;
        case AssociativityType::FOUR_WAY:
            ways = 4;
            num_sets = capacity / 4;
            break;
        case AssociativityType::FULLY_ASSOCIATIVE:
            ways = capacity;
            num_sets = 1;
            break;
    }
    
    // Initialize cache structure: cache[set][way]
    cache.resize(num_sets);
    for (int i = 0; i < num_sets; i++) {
        cache[i].resize(ways);
    }
}

// Read operation - returns true if HIT, false if MISS
bool Cache::read(size_t address) {
    access_counter++;
    
    int set_index = getSetIndex(address);
    size_t tag = getTag(address);
    
    // Check if tag is in this set (search for hit)
    for (int way = 0; way < ways; way++) {
        if (cache[set_index][way].valid && cache[set_index][way].tag == tag) {
            // Cache HIT!
            hits++;
            
            // Update access time for LRU
            if (replacement_policy == ReplacementPolicy::LRU) {
                cache[set_index][way].last_access_time = access_counter;
            }
            
            return true;
        }
    }
    
    // Cache MISS
    misses++;
    return false;
}

// Write operation - returns true if HIT, false if MISS
bool Cache::write(size_t address) {
    access_counter++;
    writes++;
    
    int set_index = getSetIndex(address);
    size_t tag = getTag(address);
    
    // Check if tag is in this set (search for hit)
    for (int way = 0; way < ways; way++) {
        if (cache[set_index][way].valid && cache[set_index][way].tag == tag) {
            // Write HIT!
            write_hits++;
            hits++;
            
            // Update access time for LRU
            if (replacement_policy == ReplacementPolicy::LRU) {
                cache[set_index][way].last_access_time = access_counter;
            }
            
            // Handle write policy
            if (write_policy == WritePolicy::WRITE_BACK) {
                // Mark as dirty (will write to memory on eviction)
                cache[set_index][way].dirty = true;
            }
            // For WRITE_THROUGH, write happens to memory immediately (handled by hierarchy)
            
            return true;
        }
    }
    
    // Write MISS
    write_misses++;
    misses++;
    
    // Write-allocate: Bring block into cache on write miss
    // (This is standard behavior for most caches)
    int victim_way = findVictimInSet(set_index);
    
    // If evicting a dirty line (write-back only), need to write back
    if (cache[set_index][victim_way].valid && 
        cache[set_index][victim_way].dirty &&
        write_policy == WritePolicy::WRITE_BACK) {
        writebacks++;
    }
    
    // Insert new entry
    cache[set_index][victim_way].valid = true;
    cache[set_index][victim_way].tag = tag;
    cache[set_index][victim_way].insertion_order = next_insertion_order++;
    cache[set_index][victim_way].last_access_time = access_counter;
    
    // Set dirty bit based on write policy
    if (write_policy == WritePolicy::WRITE_BACK) {
        cache[set_index][victim_way].dirty = true;  // Dirty immediately
    } else {
        cache[set_index][victim_way].dirty = false; // Clean (written through)
    }
    
    return false;
}

// Insert (for hierarchy updates)
void Cache::insert(size_t address, bool is_dirty) {
    int set_index = getSetIndex(address);
    size_t tag = getTag(address);
    
    // For write-through policy, data is always clean (written through to memory)
    // So we ignore the is_dirty flag for write-through caches
    bool actual_dirty = (write_policy == WritePolicy::WRITE_BACK) ? is_dirty : false;
    
    // Check if already present
    for (int way = 0; way < ways; way++) {
        if (cache[set_index][way].valid && cache[set_index][way].tag == tag) {
            // Update access time for LRU
            if (replacement_policy == ReplacementPolicy::LRU) {
                cache[set_index][way].last_access_time = ++access_counter;
            }
            // Update dirty bit if needed (only for write-back)
            if (actual_dirty) {
                cache[set_index][way].dirty = true;
            }
            return;
        }
    }
    
    // Not present, insert
    int victim_way = findVictimInSet(set_index);
    
    // Check if evicting dirty line
    if (cache[set_index][victim_way].valid && 
        cache[set_index][victim_way].dirty &&
        write_policy == WritePolicy::WRITE_BACK) {
        writebacks++;
    }
    
    cache[set_index][victim_way].valid = true;
    cache[set_index][victim_way].tag = tag;
    cache[set_index][victim_way].dirty = actual_dirty;
    cache[set_index][victim_way].insertion_order = next_insertion_order++;
    cache[set_index][victim_way].last_access_time = ++access_counter;
}

// Evict and return if dirty
bool Cache::evict(size_t address, bool& was_dirty) {
    int set_index = getSetIndex(address);
    size_t tag = getTag(address);
    
    for (int way = 0; way < ways; way++) {
        if (cache[set_index][way].valid && cache[set_index][way].tag == tag) {
            was_dirty = cache[set_index][way].dirty;
            cache[set_index][way].valid = false;
            cache[set_index][way].dirty = false;
            
            if (was_dirty && write_policy == WritePolicy::WRITE_BACK) {
                writebacks++;
            }
            
            return true;
        }
    }
    
    was_dirty = false;
    return false;
}

// Display cache statistics
void Cache::displayStats() const {
    cout << name << " Statistics:\n";
    cout << "  Capacity: " << capacity << " lines\n";
    cout << "  Block size: " << block_size << " bytes\n";
    cout << "  Associativity: ";
    switch(associativity) {
        case AssociativityType::DIRECT_MAPPED:
            cout << "Direct-mapped (1-way)\n";
            break;
        case AssociativityType::TWO_WAY:
            cout << "2-way set associative\n";
            break;
        case AssociativityType::FOUR_WAY:
            cout << "4-way set associative\n";
            break;
        case AssociativityType::FULLY_ASSOCIATIVE:
            cout << "Fully associative\n";
            break;
    }
    cout << "  Sets: " << num_sets << ", Ways: " << ways << "\n";
    cout << "  Replacement Policy: " << (replacement_policy == ReplacementPolicy::FIFO ? "FIFO" : "LRU") << "\n";
    cout << "  Write Policy: " << (write_policy == WritePolicy::WRITE_THROUGH ? "Write-Through" : "Write-Back") << "\n";
    cout << "  Hits: " << hits << "\n";
    cout << "  Misses: " << misses << "\n";
    cout << "  Total accesses: " << (hits + misses) << "\n";
    cout << "  Hit ratio: " << fixed << setprecision(2) << getHitRatio() << "%\n";
    
    if (writes > 0) {
        cout << "  Writes: " << writes << " (Hits: " << write_hits 
             << ", Misses: " << write_misses << ")\n";
    }
    
    if (write_policy == WritePolicy::WRITE_BACK) {
        cout << "  Write-backs to memory: " << writebacks << "\n";
    }
}
    
// Get hit ratio
double Cache::getHitRatio() const {
    int total = hits + misses;
    if (total == 0) return 0.0;
    return ((double)hits / total) * 100.0;
}

// Get statistics
int Cache::getHits() const { return hits; }
int Cache::getMisses() const { return misses; }
int Cache::getTotalAccesses() const { return hits + misses; }
int Cache::getWritebacks() const { return writebacks; }
WritePolicy Cache::getWritePolicy() const { return write_policy; }
    
// Clear cache
void Cache::clear() {
    for (int set = 0; set < num_sets; set++) {
        for (int way = 0; way < ways; way++) {
            cache[set][way].valid = false;
            cache[set][way].dirty = false;
        }
    }
    hits = 0;
    misses = 0;
    writes = 0;
    write_hits = 0;
    write_misses = 0;
    writebacks = 0;
    next_insertion_order = 0;
    access_counter = 0;
}
    
// Display cache contents
void Cache::displayContents() const {
    cout << name << " Contents:\n";
    for (int set = 0; set < num_sets; set++) {
        cout << "  Set " << set << ":\n";
        for (int way = 0; way < ways; way++) {
            cout << "    Way " << way << ": ";
            if (cache[set][way].valid) {
                cout << "Tag=" << cache[set][way].tag 
                     << (cache[set][way].dirty ? " [DIRTY]" : " [CLEAN]")
                     << " (order=" << cache[set][way].insertion_order;
                if (replacement_policy == ReplacementPolicy::LRU) {
                    cout << ", lru=" << cache[set][way].last_access_time;
                }
                cout << ")\n";
            } else {
                cout << "EMPTY\n";
            }
        }
    }
}

// ==================== CACHE HIERARCHY ====================

// Constructor
CacheHierarchy::CacheHierarchy(
    int l1_lines, int l1_block, AssociativityType l1_assoc, ReplacementPolicy l1_repl, WritePolicy l1_write,
    int l2_lines, int l2_block, AssociativityType l2_assoc, ReplacementPolicy l2_repl, WritePolicy l2_write,
    int l3_lines, int l3_block, AssociativityType l3_assoc, ReplacementPolicy l3_repl, WritePolicy l3_write)
    : total_accesses(0), total_reads(0), total_writes(0),
      l1_hits(0), l2_hits(0), l3_hits(0), memory_accesses(0), memory_writes(0),
      l1_penalty(1), l2_penalty(10), l3_penalty(50), memory_penalty(100), 
      total_penalty_cycles(0) {
    
    l1 = new Cache("L1", l1_lines, l1_block, l1_assoc, l1_repl, l1_write);
    
    if (l2_lines > 0) {
        l2 = new Cache("L2", l2_lines, l2_block, l2_assoc, l2_repl, l2_write);
        has_l2 = true;
    } else {
        l2 = nullptr;
        has_l2 = false;
    }
    
    if (l3_lines > 0) {
        l3 = new Cache("L3", l3_lines, l3_block, l3_assoc, l3_repl, l3_write);
        has_l3 = true;
    } else {
        l3 = nullptr;
        has_l3 = false;
    }
    
    cout << "\n========================================\n";
    cout << "Cache hierarchy initialized\n";
    cout << "========================================\n";
}
    
// Destructor
CacheHierarchy::~CacheHierarchy() {
    delete l1;
    if (l2 != nullptr) delete l2;
    if (l3 != nullptr) delete l3;
}

// Read operation through hierarchy
bool CacheHierarchy::read(size_t address, bool verbose) {
    total_accesses++;
    total_reads++;
    int penalty = 0;
    
    if (verbose) cout << "\nReading address " << address << ":\n";
    
    // Step 1: Try L1
    if (l1->read(address)) {
        l1_hits++;
        penalty = 1;
        if (verbose) cout << "  [OK] L1 HIT (1 cycle)\n";
        total_penalty_cycles += penalty;
        return false;  // No memory access needed
    }
    
    // L1 miss
    penalty += l1_penalty;
    if (verbose) cout << "  [X] L1 MISS (+" << l1_penalty << " cycles)";
    
    // Step 2: Try L2 (if exists)
    if (has_l2) {
        if (verbose) cout << " -> checking L2...\n";
        
        if (l2->read(address)) {
            l2_hits++;
            penalty += 10;
            if (verbose) cout << "  [OK] L2 HIT (10 cycles, total: " << penalty << " cycles)\n";
            l1->insert(address);
            if (verbose) cout << "  -> Updated L1\n";
            total_penalty_cycles += penalty;
            return false;
        }
        
        // L2 miss
        penalty += l2_penalty;
        if (verbose) cout << "  [X] L2 MISS (+" << l2_penalty << " cycles)";
    }
    
    // Step 3: Try L3 (if exists)
    if (has_l3) {
        if (verbose) cout << " -> checking L3...\n";
        
        if (l3->read(address)) {
            l3_hits++;
            penalty += 50;
            if (verbose) cout << "  [OK] L3 HIT (50 cycles, total: " << penalty << " cycles)\n";
            if (has_l2) l2->insert(address);
            l1->insert(address);
            if (verbose) cout << "  -> Updated caches\n";
            total_penalty_cycles += penalty;
            return false;
        }
        
        penalty += l3_penalty;
        if (verbose) cout << "  [X] L3 MISS (+" << l3_penalty << " cycles)";
    }
    
    if (verbose) cout << " -> accessing MEMORY\n";
    
    // Step 4: Memory access
    memory_accesses++;
    penalty += memory_penalty;
    if (verbose) cout << "  -> MEMORY ACCESS (+" << memory_penalty << " cycles, total: " << penalty << " cycles)\n";
    
    // Update all caches
    if (has_l3) {
        l3->insert(address);
        if (verbose) cout << "  -> Updated L3\n";
    }
    if (has_l2) {
        l2->insert(address);
        if (verbose) cout << "  -> Updated L2\n";
    }
    l1->insert(address);
    if (verbose) cout << "  -> Updated L1\n";
    
    total_penalty_cycles += penalty;
    return true;  // Memory accessed
}

// Write operation through hierarchy
bool CacheHierarchy::write(size_t address, bool verbose) {
    total_accesses++;
    total_writes++;
    int penalty = 0;
    
    if (verbose) cout << "\nWriting to address " << address << ":\n";
    
    // Step 1: Try L1
    if (l1->write(address)) {
        l1_hits++;
        penalty = 1;
        
        // For write-through, every write goes to memory immediately
        if (l1->getWritePolicy() == WritePolicy::WRITE_THROUGH) {
            memory_writes++;
            if (verbose) cout << "  [OK] L1 WRITE HIT (1 cycle) -> Write-through to memory\n";
        } else {
            // Write-back: write stays in cache (dirty bit set)
            if (verbose) cout << "  [OK] L1 WRITE HIT (1 cycle) -> Cached (dirty)\n";
        }
        
        total_penalty_cycles += penalty;
        return false;  // No memory read needed for cache hit
    }
    
    // L1 miss
    penalty += l1_penalty;
    if (verbose) cout << "  [X] L1 WRITE MISS (+" << l1_penalty << " cycles)";
    
    // Step 2: Try L2 (if exists)
    if (has_l2) {
        if (verbose) cout << " -> checking L2...\n";
        
        if (l2->write(address)) {
            l2_hits++;
            penalty += 10;
            
            // For write-through, propagate to memory
            if (l1->getWritePolicy() == WritePolicy::WRITE_THROUGH) {
                memory_writes++;
                if (verbose) cout << "  [OK] L2 WRITE HIT (10 cycles) -> Write-through to memory\n";
            } else {
                if (verbose) cout << "  [OK] L2 WRITE HIT (10 cycles) -> Cached (dirty)\n";
            }
            
            l1->insert(address, l1->getWritePolicy() == WritePolicy::WRITE_BACK);
            if (verbose) cout << "  -> Updated L1\n";
            total_penalty_cycles += penalty;
            return false;
        }
        
        // L2 miss
        penalty += l2_penalty;
        if (verbose) cout << "  [X] L2 WRITE MISS (+" << l2_penalty << " cycles)";
    }
    
    // Step 3: Try L3 (if exists)
    if (has_l3) {
        if (verbose) cout << " -> checking L3...\n";
        
        if (l3->write(address)) {
            l3_hits++;
            penalty += 50;
            
            // For write-through, propagate to memory
            bool is_write_through = (l1->getWritePolicy() == WritePolicy::WRITE_THROUGH);
            if (is_write_through) {
                memory_writes++;
                if (verbose) cout << "  [OK] L3 WRITE HIT (50 cycles) -> Write-through to memory\n";
            } else {
                if (verbose) cout << "  [OK] L3 WRITE HIT (50 cycles) -> Cached (dirty)\n";
            }
            
            bool mark_dirty = !is_write_through;
            if (has_l2) l2->insert(address, mark_dirty);
            l1->insert(address, mark_dirty);
            if (verbose) cout << "  -> Updated caches\n";
            total_penalty_cycles += penalty;
            return false;
        }
        
        penalty += l3_penalty;
        if (verbose) cout << "  [X] L3 WRITE MISS (+" << l3_penalty << " cycles)";
    }
    
    if (verbose) cout << " -> accessing MEMORY\n";
    
    // Step 4: Memory access (write-allocate policy)
    // On write miss, we need to fetch the block from memory first
    memory_accesses++;  // This is a memory READ to fetch the block
    penalty += memory_penalty;
    
    // Determine if we also need to write to memory based on L1's write policy
    bool is_write_through = (l1->getWritePolicy() == WritePolicy::WRITE_THROUGH);
    
    if (is_write_through) {
        // Write-through: Also write to memory
        memory_writes++;
        if (verbose) cout << "  -> MEMORY READ+WRITE (" << memory_penalty << " cycles, total: " << penalty << " cycles)\n";
        if (verbose) cout << "  -> Write-through: data written to memory\n";
    } else {
        // Write-back: Only read block, write stays in cache
        if (verbose) cout << "  -> MEMORY READ (fetch block) (" << memory_penalty << " cycles, total: " << penalty << " cycles)\n";
        if (verbose) cout << "  -> Write-back: data cached as dirty\n";
    }
    
    // Update all caches with dirty flag (for write-back) or clean (for write-through)
    bool mark_dirty = !is_write_through;  // Only dirty for write-back
    
    if (has_l3) {
        l3->insert(address, mark_dirty);
        if (verbose) cout << "  -> Updated L3" << (mark_dirty ? " (dirty)" : " (clean)") << "\n";
    }
    if (has_l2) {
        l2->insert(address, mark_dirty);
        if (verbose) cout << "  -> Updated L2" << (mark_dirty ? " (dirty)" : " (clean)") << "\n";
    }
    l1->insert(address, mark_dirty);
    if (verbose) cout << "  -> Updated L1" << (mark_dirty ? " (dirty)" : " (clean)") << "\n";
    
    total_penalty_cycles += penalty;
    return true;  // Memory accessed
}

// Generic access (defaults to read)
bool CacheHierarchy::access(size_t address, bool verbose) {
    return read(address, verbose);
}
    
// Display all statistics
void CacheHierarchy::displayStats() const {
    cout << "\n========================================\n";
    cout << "   CACHE HIERARCHY STATISTICS\n";
    cout << "========================================\n\n";
    
    l1->displayStats();
    
    if (has_l2) {
        cout << "\n";
        l2->displayStats();
    }
    
    if (has_l3) {
        cout << "\n";
        l3->displayStats();
    }
    
    cout << "\n========================================\n";
    cout << "Overall Statistics:\n";
    cout << "  Total accesses: " << total_accesses << "\n";
    cout << "  Total reads: " << total_reads << "\n";
    cout << "  Total writes: " << total_writes << "\n";
    cout << "  L1 hits: " << l1_hits << "\n";
    cout << "  L2 hits: " << l2_hits << "\n";
    if (has_l3) {
        cout << "  L3 hits: " << l3_hits << "\n";
    }
    cout << "  Memory accesses: " << memory_accesses << "\n";
    cout << "  Memory writes: " << memory_writes << "\n";
    
    double overall_hit_ratio = 0.0;
    if (total_accesses > 0) {
        int total_hits = l1_hits + l2_hits;
        if (has_l3) total_hits += l3_hits;
        overall_hit_ratio = ((double)total_hits / total_accesses) * 100.0;
    }
    cout << "  Overall hit ratio: " << fixed << setprecision(2) 
              << overall_hit_ratio << "%\n";
    
    // Total write-backs
    int total_writebacks = l1->getWritebacks();
    if (has_l2) total_writebacks += l2->getWritebacks();
    if (has_l3) total_writebacks += l3->getWritebacks();
    
    if (total_writebacks > 0) {
        cout << "  Total write-backs: " << total_writebacks << "\n";
    }
    
    cout << "\nMiss Penalty Analysis:\n";
    cout << "  Total penalty cycles: " << total_penalty_cycles << "\n";
    if (total_accesses > 0) {
        double avg_penalty = (double)total_penalty_cycles / total_accesses;
        cout << "  Average cycles per access: " << fixed << setprecision(2) << avg_penalty << "\n";
    }
    cout << "  (L1 hit=1, L2 hit=10, L3 hit=50, Memory=100 cycles)\n";
    
    cout << "========================================\n";
}
    
// Clear all caches
void CacheHierarchy::clearAll() {
    l1->clear();
    if (has_l2) l2->clear();
    if (has_l3) l3->clear();
    total_accesses = 0;
    total_reads = 0;
    total_writes = 0;
    l1_hits = 0;
    l2_hits = 0;
    l3_hits = 0;
    memory_accesses = 0;
    memory_writes = 0;
    total_penalty_cycles = 0;
    cout << "All caches cleared\n";
}
    
// Display cache contents
void CacheHierarchy::displayContents() const {
    l1->displayContents();
    if (has_l2) {
        cout << "\n";
        l2->displayContents();
    }
    if (has_l3) {
        cout << "\n";
        l3->displayContents();
    }
}

// ==================== HELPER FUNCTIONS ====================

AssociativityType parseAssociativity(string assoc_str) {
    if (assoc_str == "direct") return AssociativityType::DIRECT_MAPPED;
    if (assoc_str == "2way") return AssociativityType::TWO_WAY;
    if (assoc_str == "4way") return AssociativityType::FOUR_WAY;
    if (assoc_str == "fully") return AssociativityType::FULLY_ASSOCIATIVE;
    return AssociativityType::FULLY_ASSOCIATIVE;
}

WritePolicy parseWritePolicy(string write_str) {
    if (write_str == "wt" || write_str == "write-through" || write_str == "writethrough") {
        return WritePolicy::WRITE_THROUGH;
    }
    if (write_str == "wb" || write_str == "write-back" || write_str == "writeback") {
        return WritePolicy::WRITE_BACK;
    }
    return WritePolicy::WRITE_THROUGH;  // Default
}
