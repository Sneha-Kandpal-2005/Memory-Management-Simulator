#ifndef VIRTUAL_MEMORY_SIMULATOR_H
#define VIRTUAL_MEMORY_SIMULATOR_H

#include <iostream>
#include <vector>
#include <string>
#include <cstddef>

using namespace std;

// ==================== PAGE TABLE ENTRY ====================

struct PageTableEntry {
    bool valid;              // Is page in physical memory?
    int frame_number;        // Physical frame number (-1 if not in memory)
    bool dirty;              // Has page been modified?
    int last_access_time;    // For LRU replacement
    int load_time;           // For FIFO replacement
    int access_count;        // For statistics
    
    PageTableEntry() 
        : valid(false), frame_number(-1), dirty(false),
          last_access_time(0), load_time(0), access_count(0) {}
};

// ==================== PAGE REPLACEMENT POLICY ENUM ====================

enum class PageReplacementPolicy {
    FIFO,
    LRU
};

// ==================== VIRTUAL MEMORY SIMULATOR CLASS ====================

class VirtualMemorySimulator {
private:
    // Configuration
    size_t virtual_memory_size;
    size_t physical_memory_size;
    size_t page_size;
    
    int num_virtual_pages;
    int num_physical_frames;
    
    PageReplacementPolicy policy;
    
    // Page table (one entry per virtual page)
    vector<PageTableEntry> page_table;
    
    // Frame allocation tracking
    vector<int> frame_to_page;   // Maps frame number to page number (-1 if free)
    vector<bool> frame_used;     // Is frame occupied?
    
    // Statistics
    int page_faults;
    int page_hits;
    int total_accesses;
    int disk_reads;
    int disk_writes;
    int current_time;
    
    bool verbose;
    
    // Helper functions
    void handlePageFault(int page_number);
    int findFreeFrame();
    int selectVictimPage();
    int evictPage(int page_number);
    void loadPage(int page_number, int frame_number);
    
public:
    // Constructor
    VirtualMemorySimulator(size_t vm_size, size_t pm_size, size_t pg_size, string policy_str = "fifo");
    
    // Main operations
    void setReplacementPolicy(string policy_str);
    void setVerbose(bool v);
    size_t translateAddress(size_t virtual_address);
    void access(size_t virtual_address);
    
    // Display functions
    void displayPageTable();
    void displayFrames();
    void displayStats();
    void clearStats();
    void reset();
};

#endif // VIRTUAL_MEMORY_SIMULATOR_H