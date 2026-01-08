#ifndef CACHE_SIMULATOR_H
#define CACHE_SIMULATOR_H

#include <iostream>
#include <vector>
#include <string>

using namespace std;

// ==================== DATA STRUCTURES ====================

// Cache replacement policy
enum class ReplacementPolicy {
    FIFO,  // First In First Out
    LRU    // Least Recently Used
};

// Cache write policy (NEW)
enum class WritePolicy {
    WRITE_THROUGH,  // Write to cache and memory immediately
    WRITE_BACK      // Write to cache only, write to memory on eviction
};

// Cache associativity type
enum class AssociativityType {
    DIRECT_MAPPED,      // 1-way: Each address maps to exactly one line
    TWO_WAY,            // 2-way set associative
    FOUR_WAY,           // 4-way set associative
    FULLY_ASSOCIATIVE   // Any address can go anywhere
};

// Cache Line (Entry)
struct CacheLine {
    bool valid;             // Is this entry valid?
    size_t tag;             // Address tag
    bool dirty;             // Modified but not written to memory (for write-back)
    int insertion_order;    // For FIFO
    int last_access_time;   // For LRU
    
    CacheLine() : valid(false), tag(0), dirty(false), insertion_order(0), last_access_time(0) {}
};

// ==================== CACHE CLASS (Internal Helper) ====================

class Cache {
private:
    string name;                           // Cache name (L1, L2, L3)
    int capacity;                          // Total number of cache lines
    int block_size;                        // Block size in bytes
    AssociativityType associativity;       // Type of associativity
    ReplacementPolicy replacement_policy;  // FIFO or LRU
    WritePolicy write_policy;              // Write-through or Write-back
    
    int num_sets;                          // Number of sets
    int ways;                              // Ways per set (associativity)
    
    vector<vector<CacheLine>> cache;       // cache[set][way]
    
    // Tracking counters
    int next_insertion_order;              // For FIFO
    int access_counter;                    // For LRU
    
    // Statistics
    int hits;
    int misses;
    int writes;                            // Total write operations
    int write_hits;                        // Writes that hit
    int write_misses;                      // Writes that miss
    int writebacks;                        // Write-backs to memory (dirty evictions)
    
    // Helper functions
    int getSetIndex(size_t address);
    size_t getTag(size_t address);
    int findVictimInSet(int set_index);
    int findFIFOVictimInSet(int set_index);
    int findLRUVictimInSet(int set_index);
    
public:
    Cache(string cache_name, int total_lines, int blk_size, 
          AssociativityType assoc, ReplacementPolicy repl_pol, WritePolicy wr_pol);

    // Read operation
    bool read(size_t address);
    
    // Write operation 
    bool write(size_t address);
    
    // Insert (for hierarchy updates)
    void insert(size_t address, bool is_dirty = false);
    
    // Evict and return if dirty (for write-back policy) 
    bool evict(size_t address, bool& was_dirty);
    
    // bool access(size_t address);
    // void insert(size_t address);

    // Display functions
    void displayStats() const;
    double getHitRatio() const;
    int getHits() const;
    int getMisses() const;
    int getTotalAccesses() const;
    int getWritebacks() const; 
    WritePolicy getWritePolicy() const;  // Get the write policy for this cache
    void clear();
    void displayContents() const;
};

// ==================== CACHE HIERARCHY CLASS ====================

class CacheHierarchy {
private:
    Cache* l1;
    Cache* l2;
    Cache* l3;
    
    bool has_l2;
    bool has_l3;
    
    // Overall statistics
    int total_accesses;
    int total_reads;                    
    int total_writes;                   
    int l1_hits;
    int l2_hits;
    int l3_hits;
    int memory_accesses;
    int memory_writes;                  // Writes to main memory
    
    // Miss penalties (in cycles)
    int l1_penalty;
    int l2_penalty;
    int l3_penalty;
    int memory_penalty;
    
    int total_penalty_cycles;
    
public:
    CacheHierarchy(int l1_lines, int l1_block, AssociativityType l1_assoc, 
                   ReplacementPolicy l1_repl, WritePolicy l1_write,
                   int l2_lines, int l2_block, AssociativityType l2_assoc, 
                   ReplacementPolicy l2_repl, WritePolicy l2_write,
                   int l3_lines, int l3_block, AssociativityType l3_assoc, 
                   ReplacementPolicy l3_repl, WritePolicy l3_write);
    ~CacheHierarchy();
    
    // Main operations
    bool read(size_t address, bool verbose = true);      // explicit read
    bool write(size_t address, bool verbose = true);     // explicit write
    bool access(size_t address, bool verbose = true);    // Generic access (read)
    bool has_l2_level() const {return has_l2; }
    bool has_l3_level() const {return has_l3; }
    
    // Display functions
    void displayStats() const;
    void displayContents() const;
    void clearAll();
};

// ==================== HELPER FUNCTIONS ====================

AssociativityType parseAssociativity(string assoc_str);
WritePolicy parseWritePolicy(string write_str);  

#endif // CACHE_SIMULATOR_H