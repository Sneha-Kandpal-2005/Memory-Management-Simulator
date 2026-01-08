/*
 * ================================================================
 * INTEGRATED MEMORY MANAGEMENT SIMULATOR
 * ================================================================
 * 
 * Unified Flow (Automatic Integration):
 *   Virtual Address -> Page Table -> Physical Address -> Cache -> Memory
 * 
 * User chooses:
 *   1. Memory allocator type: Classic (First/Best/Worst Fit) OR Buddy
 *   2. Whether to enable Virtual Memory (paging)
 *   3. Whether to enable Cache hierarchy
 * 
 * The system automatically follows the correct flow:
 *   - If VM enabled: All accesses go through address translation first
 *   - If Cache enabled: All physical addresses checked in cache
 *   - Memory allocator manages the underlying physical memory
 * 
 * ================================================================
 */

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#include "memory_allocator.h"
#include "buddy_allocator.h"
#include "cache_simulator.h"
#include "virtual_memory_simulator.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

void setupConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
}

// ================================================================
// UNIFIED MEMORY MANAGEMENT SYSTEM
// ================================================================

class UnifiedMemorySystem {
private:
    // Memory Allocator (choose one)
    MemoryManager* classic_allocator;
    BuddyAllocator* buddy_allocator;
    bool use_buddy;
    
    // Optional components
    VirtualMemorySimulator* vm_simulator;
    CacheHierarchy* cache_hierarchy;
    
    // System configuration
    bool vm_enabled;
    bool cache_enabled;
    bool verbose;
    
    // Physical memory size
    size_t physical_memory_size;
    
public:
    UnifiedMemorySystem()
        : classic_allocator(nullptr),
          buddy_allocator(nullptr),
          use_buddy(false),
          vm_simulator(nullptr),
          cache_hierarchy(nullptr),
          vm_enabled(false),
          cache_enabled(false),
          verbose(false),
          physical_memory_size(0) {}
    
    ~UnifiedMemorySystem() {
        cleanup();
    }
    
    void cleanup() {
        delete classic_allocator;
        delete buddy_allocator;
        delete vm_simulator;
        delete cache_hierarchy;
        
        classic_allocator = nullptr;
        buddy_allocator = nullptr;
        vm_simulator = nullptr;
        cache_hierarchy = nullptr;
    }
    
    // ================================================================
    // INITIALIZATION
    // ================================================================
    
    void initializeMemory(size_t size, bool use_buddy_system = false) {
        physical_memory_size = size;
        use_buddy = use_buddy_system;
        
        cout << "\n========================================\n";
        cout << "Initializing Memory Allocator\n";
        cout << "========================================\n";
        
        if (use_buddy) {
            // Buddy allocator requires power-of-2 size
            if (!isPowerOfTwo(size)) {
                cout << "Warning: Buddy allocator requires power-of-2 size.\n";
                size = nextPowerOfTwo(size);
                cout << "Adjusting to: " << size << " bytes\n";
                physical_memory_size = size;
            }
            
            size_t min_block = 16;  // Default minimum block size
            delete buddy_allocator;
            buddy_allocator = new BuddyAllocator(size, min_block);
            
            cout << "Memory Allocator: BUDDY SYSTEM\n";
        } else {
            delete classic_allocator;
            classic_allocator = new MemoryManager(size);
            
            cout << "Memory Allocator: CLASSIC (First/Best/Worst Fit)\n";
        }
        
        cout << "Physical Memory: " << size << " bytes\n";
        cout << "========================================\n";
    }
    
    void initializeVirtualMemory(size_t vm_size, size_t page_size, string policy = "fifo") {
        if (physical_memory_size == 0) {
            cout << "Error: Initialize physical memory first!\n";
            return;
        }
        
        cout << "\n========================================\n";
        cout << "Initializing Virtual Memory\n";
        cout << "========================================\n";
        
        delete vm_simulator;
        vm_simulator = new VirtualMemorySimulator(vm_size, physical_memory_size, page_size, policy);
        vm_enabled = true;
        
        cout << "Virtual Memory: ENABLED\n";
        cout << "Flow: Virtual Address -> Page Table -> Physical Address\n";
        cout << "========================================\n";
    }
    
    void initializeCache(int l1_lines, int l1_block, string l1_assoc_str, string l1_pol_str, string l1_write_str,
                         int l2_lines, int l2_block, string l2_assoc_str, string l2_pol_str, string l2_write_str,
                         int l3_lines, int l3_block, string l3_assoc_str, string l3_pol_str, string l3_write_str) {  
        cout << "\n========================================\n";
        cout << "Initializing Cache Hierarchy\n";
        cout << "========================================\n";
      
        AssociativityType l1_assoc = parseAssociativity(l1_assoc_str);
        AssociativityType l2_assoc = parseAssociativity(l2_assoc_str);
        AssociativityType l3_assoc = parseAssociativity(l3_assoc_str);
                
        ReplacementPolicy l1_pol = (l1_pol_str == "fifo") ? ReplacementPolicy::FIFO : ReplacementPolicy::LRU;
        ReplacementPolicy l2_pol = (l2_pol_str == "fifo") ? ReplacementPolicy::FIFO : ReplacementPolicy::LRU;
        ReplacementPolicy l3_pol = (l3_pol_str == "fifo") ? ReplacementPolicy::FIFO : ReplacementPolicy::LRU;
        
        WritePolicy l1_write = parseWritePolicy(l1_write_str);
        WritePolicy l2_write = parseWritePolicy(l2_write_str);
        WritePolicy l3_write = parseWritePolicy(l3_write_str);

        delete cache_hierarchy;
        cache_hierarchy = new CacheHierarchy(
            l1_lines, l1_block, l1_assoc, l1_pol, l1_write,
            l2_lines, l2_block, l2_assoc, l2_pol, l2_write,
            l3_lines, l3_block, l3_assoc, l3_pol, l3_write
        );
        cache_enabled = true;
        
        cout << "Cache Hierarchy: ENABLED\n";
        cout << "Flow: Physical Address -> L1";
        if(l2_lines) cout<<" -> L2";
        if(l3_lines) cout<<" -> L3";
        cout<<"\n";
        cout << "========================================\n";
    }
    
    // ================================================================
    // UNIFIED MEMORY ACCESS
    // ================================================================
    
    /**
     * The core unified access method that automatically follows the correct flow:
     * 1. If VM enabled: Virtual -> Physical translation
     * 2. If Cache enabled: Check cache hierarchy
     * 3. Access physical memory
     */
    void accessMemory(size_t address, bool is_write = false) {
        size_t physical_address = address;
        
        cout << "\n+==========================================================+\n";
        cout << "|                  UNIFIED MEMORY ACCESS                   |\n";
        cout << "+==========================================================+\n";
        
        // ============================================================
        // STEP 1: VIRTUAL MEMORY (if enabled)
        // ============================================================
        if (vm_enabled && vm_simulator) {
            cout << "\n  [STEP 1] VIRTUAL MEMORY - Address Translation\n";
            cout << "  ---------------------------------------------------\n";
            cout << "  Input: Virtual Address 0x" << hex << address << dec << " (" << address << ")\n";
            
            physical_address = vm_simulator->translateAddress(address);
            
            if (physical_address == (size_t)-1) {
                cout << "\n  [X] Address translation FAILED\n";
                cout << "  Access terminated.\n";
                return;
            }
            
            cout << "  [OK] Translation successful\n";
            cout << "  Output: Physical Address 0x" << hex << physical_address << dec << " (" << physical_address << ")\n";
        } else {
            cout << "\n  [STEP 1] VIRTUAL MEMORY: Disabled\n";
            cout << "  Using direct physical addressing\n";
            cout << "  Physical Address: 0x" << hex << address << dec << " (" << address << ")\n";
        }
        
        // ============================================================
        // STEP 2: CACHE HIERARCHY (if enabled)
        // ============================================================
        bool all_cache_miss=true;
        if (cache_enabled && cache_hierarchy) {
            cout << "\n  [STEP 2] CACHE HIERARCHY - Multi-level Cache Check\n";
            cout << "  ---------------------------------------------------\n";
            cout << "  Operation: " << (is_write ? "WRITE" : "READ") << "\n";
            cout << "  Checking L1";
            if(cache_hierarchy->has_l2_level()) cout<<" -> L2";
            if(cache_hierarchy->has_l3_level()) cout<<" -> L3";
            cout<<" -> Memory...\n\n";
            
            if (is_write) {
                all_cache_miss = cache_hierarchy->write(physical_address, verbose);
            } else {
                all_cache_miss = cache_hierarchy->read(physical_address, verbose);
            }
        } else {
            cout << "\n  [STEP 2] CACHE HIERARCHY: Disabled\n";
            cout << "  Direct memory access\n";
        }
        
        // ============================================================
        // STEP 3: PHYSICAL MEMORY ACCESS
        // ============================================================
        if (all_cache_miss) {
            cout << "\n  [STEP 3] PHYSICAL MEMORY - Final Access\n";
            cout << "  ---------------------------------------------------\n";
            cout << "  " << (is_write ? "Writing to" : "Reading from") << " physical memory at 0x" << hex << physical_address << dec << "\n";
            cout << "  Memory allocator: " << (use_buddy ? "Buddy System" : "Classic") << "\n";
        }
        
        // Memory is managed by allocator, but actual data access is implicit
        cout << "\n  [OK] Memory access complete\n";
        
        // ============================================================
        // SUMMARY
        // ============================================================
        cout << "\n  +====================================================+\n";
        cout << "  |                      SUMMARY                       |\n";
        cout << "  +====================================================+\n";
        
        if (vm_enabled) {
            cout << "  Virtual Address:   0x" << hex << address << dec << " (" << address << ")\n";
            cout << "  Physical Address:  0x" << hex << physical_address << dec << " (" << physical_address << ")\n";
        } else {
            cout << "  Physical Address:  0x" << hex << physical_address << dec << " (" << physical_address << ")\n";
        }
        
        cout << "  Operation: " << (is_write ? "WRITE" : "READ") << "\n";
        cout << "  Flow: ";
        if (vm_enabled) cout << "VM Translation -> ";
        if (cache_enabled) cout << "Cache Hierarchy -> ";
        if (all_cache_miss) cout << "Physical Memory\n";
        else cout<<"\n";
        cout << "  Status: SUCCESS\n\n";
    }
    
    // ================================================================
    // MEMORY ALLOCATION/DEALLOCATION
    // ================================================================
    
    int allocate(size_t size) {
        cout << "\n========================================\n";
        cout << "Memory Allocation Request\n";
        cout << "========================================\n";
        
        if (use_buddy && buddy_allocator) {
            return buddy_allocator->allocate(size);
        } else if (classic_allocator) {
            return classic_allocator->allocate(size);
        } else {
            cout << "Error: No memory allocator initialized!\n";
            return -1;
        }
    }
    
    bool deallocate(int block_id) {
        cout << "\n========================================\n";
        cout << "Memory Deallocation Request\n";
        cout << "========================================\n";
        
        if (use_buddy && buddy_allocator) {
            return buddy_allocator->deallocate(block_id);
        } else if (classic_allocator) {
            return classic_allocator->deallocate(block_id);
        } else {
            cout << "Error: No memory allocator initialized!\n";
            return false;
        }
    }
    
    // ================================================================
    // DISPLAY FUNCTIONS
    // ================================================================
    
    void displaySystemStatus() {
        cout << "\n+==========================================================+\n";
        cout << "|                  SYSTEM CONFIGURATION                    |\n";
        cout << "+==========================================================+\n";
        cout << "\n  Memory Allocator:\n";
        if (use_buddy && buddy_allocator) {
            cout << "    Type: BUDDY SYSTEM\n";
            cout << "    Size: " << physical_memory_size << " bytes (power-of-2)\n";
            cout << "    Min Block: " << buddy_allocator->getMinBlockSize() << " bytes\n";
        } else if (classic_allocator) {
            cout << "    Type: CLASSIC ALLOCATOR\n";
            cout << "    Size: " << physical_memory_size << " bytes\n";
            cout << "    Strategy: First/Best/Worst Fit (configurable)\n";
        } else {
            cout << "    Status: NOT INITIALIZED\n";
        }
        
        cout << "\n  Virtual Memory:\n";
        if (vm_enabled && vm_simulator) {
            cout << "    Status: ENABLED\n";
            cout << "    Address Translation: Active\n";
        } else {
            cout << "    Status: DISABLED\n";
            cout << "    Using direct physical addressing\n";
        }
        
        cout << "\n  Cache Hierarchy:\n";
        if (cache_enabled && cache_hierarchy) {
            cout << "    Status: ENABLED\n";
            cout << "    Levels: L1";
            if(cache_hierarchy->has_l2_level()) cout<<", L2";
            if(cache_hierarchy->has_l3_level()) cout<<", L3";
            cout<<"\n";

        } else {
            cout << "    Status: DISABLED\n";
            cout << "    Direct memory access\n";
        }
        
        cout << "\n  Integration Flow:\n";
        cout << "    ";
        if (vm_enabled) cout << "Virtual Address -> Page Table -> ";
        cout << "Physical Address";
        if (cache_enabled) cout << " -> Cache Hierarchy";
        cout << " -> Memory\n";
        
        cout << "\n  Settings:\n";
        cout << "    Verbose Mode: " << (verbose ? "ON" : "OFF") << "\n";
        
        cout << "\n";
    }
    
    void displayAllStats() {
        cout << "\n+==========================================================+\n";
        cout << "|                COMPREHENSIVE STATISTICS                  |\n";
        cout << "+==========================================================+\n";
        
        // Memory allocator stats
        cout << "\n+--- MEMORY ALLOCATOR -----------------------------+\n";
        if (use_buddy && buddy_allocator) {
            buddy_allocator->displayStats();
        } else if (classic_allocator) {
            classic_allocator->displayStats();
        }
        
        // Virtual memory stats
        if (vm_enabled && vm_simulator) {
            cout << "\n+--- VIRTUAL MEMORY -------------------------------+\n";
            vm_simulator->displayStats();
        }
        
        // Cache stats
        if (cache_enabled && cache_hierarchy) {
            cout << "\n+--- CACHE HIERARCHY ------------------------------+\n";
            cache_hierarchy->displayStats();
        }
    }
    
    void displayMemoryLayout() {
        if (use_buddy && buddy_allocator) {
            buddy_allocator->displayAllocatedBlocks();
            buddy_allocator->displayFreeLists();
        } else if (classic_allocator) {
            classic_allocator->displayMemory();
        }
    }
    
    void displayPageTable() {
        if (vm_enabled && vm_simulator) {
            vm_simulator->displayPageTable();
        } else {
            cout << "Virtual memory not enabled\n";
        }
    }
    
    void displayCacheContents() {
        if (cache_enabled && cache_hierarchy) {
            cache_hierarchy->displayContents();
        } else {
            cout << "Cache not enabled\n";
        }
    }
    
    // ================================================================
    // CONFIGURATION
    // ================================================================
    
    void setAllocationStrategy(string strategy) {
        if (use_buddy) {
            cout << "Buddy allocator doesn't use First/Best/Worst Fit strategies\n";
            return;
        }
        
        if (!classic_allocator) {
            cout << "Classic allocator not initialized\n";
            return;
        }
        
        AllocationStrategy strat;
        if (strategy == "first_fit") strat = AllocationStrategy::FIRST_FIT;
        else if (strategy == "best_fit") strat = AllocationStrategy::BEST_FIT;
        else if (strategy == "worst_fit") strat = AllocationStrategy::WORST_FIT;
        else {
            cout << "Unknown strategy. Use: first_fit, best_fit, or worst_fit\n";
            return;
        }
        
        classic_allocator->setStrategy(strat);
    }
    
    void setPageReplacementPolicy(string policy) {
        if (vm_simulator) {
            vm_simulator->setReplacementPolicy(policy);
        } else {
            cout << "Virtual memory not initialized\n";
        }
    }
    
    void setVerbose(bool v) {
        verbose = v;
        if (vm_simulator) vm_simulator->setVerbose(v);
        cout << "Verbose mode: " << (verbose ? "ON" : "OFF") << "\n";
    }
    
    void clearAll() {
        cout << "\n========================================\n";
        cout << "Clearing entire system...\n";
        cleanup();
        vm_enabled = false;
        cache_enabled = false;
        physical_memory_size = 0;
        cout << "System cleared\n";
        cout << "========================================\n";
    }
    
private:
    bool isPowerOfTwo(size_t n) {
        return n > 0 && (n & (n - 1)) == 0;
    }
    
    size_t nextPowerOfTwo(size_t n) {
        if (n <= 1) return 1;
        size_t power = 1;
        while (power < n) power *= 2;
        return power;
    }
};

// ================================================================
// COMMAND LINE INTERFACE
// ================================================================

void printWelcome() {
    cout << "\n";
    cout << "+==========================================================+\n";
    cout << "|           UNIFIED MEMORY MANAGEMENT SIMULATOR            |\n";
    cout << "+==========================================================+\n";
    cout << "\n";
    cout << "  Automatic Integration Flow:\n";
    cout << "  Virtual Address -> Page Table -> Physical Address -> Cache -> Memory\n";
    cout << "\n";
    cout << "  Components (Enable as needed):\n";
    cout << "  • Memory Allocator: Classic OR Buddy (Required)\n";
    cout << "  • Virtual Memory: Optional (enables address translation)\n";
    cout << "  • Cache Hierarchy: Optional (enables L1/L2/L3 caching)\n";
    cout << "\n";
    cout << "  Type 'help' for commands\n";
    cout << "==========================================================\n";
}

void printHelp() {
    cout << "\n+=======================================================================+\n";
    cout << "|                         COMMAND REFERENCE                             |\n";
    cout << "+=======================================================================+\n";
    cout << "\n  +- SYSTEM INITIALIZATION ------------------------------------------+\n";
    cout << "  │ init memory <size> [buddy]                                       │\n";
    cout << "  │   Initialize physical memory allocator                           │\n";
    cout << "  │   Add 'buddy' for buddy system (min block size=16), else classic │\n";
    cout << "  │   Example: init memory 1024                                      │\n";
    cout << "  │   Example: init memory 1024 buddy                                │\n";
    cout << "  │                                                                  │\n";
    cout << "  │ init vm <vm_size> <page_size> [policy]                           │\n";
    cout << "  │   Enable virtual memory with paging                              │\n";
    cout << "  │   policy: fifo (default) or lru                                  │\n";
    cout << "  │   Example: init vm 65536 256 lru                                 │\n";
    cout << "  │                                                                  │\n";
    cout << "  │ setup cache                                                      │\n";
    cout << "  │   Interactive cache configuration wizard                         │\n";
    cout << "  │   Guides you step-by-step through L1/L2/L3 cache setup           │\n";
    cout << "  │   assoc: direct, 2way, 4way, fully                               │\n";
    cout << "  │   policy: fifo, lru | write: wt (write-through), wb (write-back) │\n";
    cout << "  +------------------------------------------------------------------+\n";
    cout << "\n  +- MEMORY OPERATIONS ----------------------------------------------+\n";
    cout << "  │ malloc <size>                 Allocate memory                    │\n";
    cout << "  │ free <block_id>               Deallocate memory                  │\n";
    cout << "  │ read <address>                Read from memory (unified flow)    │\n";
    cout << "  │ write <address>               Write to memory (unified flow)     │\n";
    cout << "  │ access <address>              Access memory (read, unified flow) │\n";
    cout << "  │ dump                          Show memory layout                 │\n";
    cout << "  +------------------------------------------------------------------+\n";
    cout << "\n  +- CONFIGURATION --------------------------------------------------+\n";
    cout << "  │ set strategy <first_fit|best_fit|worst_fit>                      │\n";
    cout << "  │   (for classic allocator only)                                   │\n";
    cout << "  │ set vm_policy <fifo|lru>                                         │\n";
    cout << "  │   (if virtual memory enabled)                                    │\n";
    cout << "  │ verbose <on|off>              Toggle detailed output             │\n";
    cout << "  +------------------------------------------------------------------+\n";
    cout << "\n  +- INFORMATION & STATISTICS ---------------------------------------+\n";
    cout << "  │ status                        Show system configuration          │\n";
    cout << "  │ stats                         Show all statistics                │\n";
    cout << "  │ page_table                    Show page table (if VM on)         │\n";
    cout << "  │ cache_contents                Show cache (if cache on)           │\n";
    cout << "  +------------------------------------------------------------------+\n";
    cout << "\n  +- SYSTEM CONTROL -------------------------------------------------+\n";
    cout << "  │ clear                         Clear entire system                │\n";
    cout << "  │ help                          Show this help                     │\n";
    cout << "  │ exit                          Exit simulator                     │\n";
    cout << "  +------------------------------------------------------------------+\n";
    cout << "\n  Note: The system automatically follows the flow:\n";
    cout << "        Virtual -> Physical -> Cache (R/W) -> Memory\n";
    cout << "        (depending on which components are enabled)\n";
    cout << "\n";
}

// Interactive cache setup wizard
void setupCacheInteractive(UnifiedMemorySystem& system) {
    cout << "\n+==============================================================+\n";
    cout << "|              CACHE CONFIGURATION WIZARD                      |\n";
    cout << "+==============================================================+\n\n";
    
    string input;
    int l1_lines, l1_block, l2_lines = 0, l2_block = 64, l3_lines = 0, l3_block = 64;
    string l1_assoc = "fully", l1_pol = "lru", l1_write = "wb";
    string l2_assoc = "fully", l2_pol = "lru", l2_write = "wb";
    string l3_assoc = "fully", l3_pol = "lru", l3_write = "wb";
    bool has_l2 = false, has_l3 = false;
    
    // ===== L1 CONFIGURATION (Required) =====
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    cout << "  L1 CACHE (Required)\n";
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    cout << "  Number of cache lines [default: 4]: ";
    getline(cin, input);
    l1_lines = input.empty() ? 4 : stoi(input);
    
    cout << "  Block size in bytes [default: 64]: ";
    getline(cin, input);
    l1_block = input.empty() ? 64 : stoi(input);
    
    cout << "  Associativity (direct/2way/4way/fully) [default: fully]: ";
    getline(cin, input);
    if (!input.empty()) l1_assoc = input;
    
    cout << "  Replacement policy (lru/fifo) [default: lru]: ";
    getline(cin, input);
    if (!input.empty()) l1_pol = input;
    
    cout << "  Write policy (wt=write-through / wb=write-back) [default: wb]: ";
    getline(cin, input);
    if (!input.empty()) l1_write = input;
    
    cout << "\n  [OK] L1: " << l1_lines << " lines, " << l1_block << "B blocks, " 
         << l1_assoc << ", " << l1_pol << ", " << l1_write << "\n\n";
    
    // ===== L2 CONFIGURATION (Optional) =====
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    cout << "  L2 CACHE (Optional)\n";
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    cout << "  Do you want an L2 cache? (y/n) [default: n]: ";
    getline(cin, input);
    has_l2 = (input == "y" || input == "Y" || input == "yes");
    
    if (has_l2) {
        cout << "  Number of cache lines [default: 8]: ";
        getline(cin, input);
        l2_lines = input.empty() ? 8 : stoi(input);
        
        cout << "  Block size in bytes [default: 64]: ";
        getline(cin, input);
        l2_block = input.empty() ? 64 : stoi(input);
        
        cout << "  Associativity (direct/2way/4way/fully) [default: fully]: ";
        getline(cin, input);
        if (!input.empty()) l2_assoc = input;
        
        cout << "  Replacement policy (lru/fifo) [default: lru]: ";
        getline(cin, input);
        if (!input.empty()) l2_pol = input;
        
        cout << "  Write policy (wt=write-through / wb=write-back) [default: wb]: ";
        getline(cin, input);
        if (!input.empty()) l2_write = input;
        
        cout << "\n  [OK] L2: " << l2_lines << " lines, " << l2_block << "B blocks, " 
             << l2_assoc << ", " << l2_pol << ", " << l2_write << "\n\n";
        
        // ===== L3 CONFIGURATION (Optional, only if L2 exists) =====
        cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        cout << "  L3 CACHE (Optional)\n";
        cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        cout << "  Do you want an L3 cache? (y/n) [default: n]: ";
        getline(cin, input);
        has_l3 = (input == "y" || input == "Y" || input == "yes");
        
        if (has_l3) {
            cout << "  Number of cache lines [default: 16]: ";
            getline(cin, input);
            l3_lines = input.empty() ? 16 : stoi(input);
            
            cout << "  Block size in bytes [default: 64]: ";
            getline(cin, input);
            l3_block = input.empty() ? 64 : stoi(input);
            
            cout << "  Associativity (direct/2way/4way/fully) [default: fully]: ";
            getline(cin, input);
            if (!input.empty()) l3_assoc = input;
            
            cout << "  Replacement policy (lru/fifo) [default: lru]: ";
            getline(cin, input);
            if (!input.empty()) l3_pol = input;
            
            cout << "  Write policy (wt=write-through / wb=write-back) [default: wb]: ";
            getline(cin, input);
            if (!input.empty()) l3_write = input;
            
            cout << "\n  [OK] L3: " << l3_lines << " lines, " << l3_block << "B blocks, " 
                 << l3_assoc << ", " << l3_pol << ", " << l3_write << "\n\n";
        } else {
            cout << "  [X] L3: Disabled\n\n";
        }
    } else {
        cout << "  [X] L2: Disabled\n";
        cout << "  [X] L3: Disabled (requires L2)\n\n";
    }
    
    // ===== SUMMARY AND CONFIRM =====
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    cout << "  CONFIGURATION SUMMARY\n";
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    cout << "  L1: " << l1_lines << " lines × " << l1_block << "B = " 
         << (l1_lines * l1_block) << " bytes | " << l1_assoc << " | " << l1_pol << " | " << l1_write << "\n";
    if (has_l2) {
        cout << "  L2: " << l2_lines << " lines × " << l2_block << "B = " 
             << (l2_lines * l2_block) << " bytes | " << l2_assoc << " | " << l2_pol << " | " << l2_write << "\n";
    }
    if (has_l3) {
        cout << "  L3: " << l3_lines << " lines × " << l3_block << "B = " 
             << (l3_lines * l3_block) << " bytes | " << l3_assoc << " | " << l3_pol << " | " << l3_write << "\n";
    }
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    cout << "\n  Apply this configuration? (y/n) [default: y]: ";
    getline(cin, input);
    
    if (input.empty() || input == "y" || input == "Y" || input == "yes") {
        system.initializeCache(
            l1_lines, l1_block, l1_assoc, l1_pol, l1_write,
            l2_lines, l2_block, l2_assoc, l2_pol, l2_write,
            l3_lines, l3_block, l3_assoc, l3_pol, l3_write
        );
    } else {
        cout << "  Configuration cancelled.\n";
    }
}

void processCommand(UnifiedMemorySystem& system, const string& line) {
    istringstream iss(line);
    string cmd;
    iss >> cmd;
    
    if (cmd == "help") {
        printHelp();
    }
    else if (cmd == "setup") {
        string type;
        iss >> type;
        if (type == "cache") {
            setupCacheInteractive(system);
        } else {
            cout << "Usage: setup cache\n";
            cout << "  Launches interactive cache configuration wizard\n";
        }
    }
    else if (cmd == "status") {
        system.displaySystemStatus();
    }
    else if (cmd == "stats") {
        system.displayAllStats();
    }
    else if (cmd == "verbose") {
        string state;
        iss >> state;
        system.setVerbose(state == "on");
    }
    else if (cmd == "clear") {
        system.clearAll();
    }
    else if (cmd == "init") {
        string type;
        iss >> type;
        
        if (type == "memory") {
            size_t size;
            string mode;
            iss >> size >> mode;
            bool use_buddy = (mode == "buddy");
            system.initializeMemory(size, use_buddy);
        }
        else if (type == "vm") {
            size_t vm_size, page_size;
            string policy = "fifo";
            if (iss >> vm_size >> page_size) {
                iss >> policy;
                system.initializeVirtualMemory(vm_size, page_size, policy);
            } else {
                cout << "Usage: init vm <vm_size> <page_size> [policy]\n";
            }
        }
        else if (type == "cache") {
            int l1_lines, l1_block, l2_lines, l2_block, l3_lines, l3_block;
            string l1_assoc_str, l1_pol_str, l1_write_str;
            string l2_assoc_str, l2_pol_str, l2_write_str;
            string l3_assoc_str, l3_pol_str, l3_write_str;
            
            if (iss >> l1_lines >> l1_block >> l1_assoc_str >> l1_pol_str >> l1_write_str
                    >> l2_lines >> l2_block >> l2_assoc_str >> l2_pol_str >> l2_write_str
                    >> l3_lines >> l3_block >> l3_assoc_str >> l3_pol_str >> l3_write_str) {
                system.initializeCache(
                    l1_lines, l1_block, l1_assoc_str, l1_pol_str, l1_write_str,
                    l2_lines, l2_block, l2_assoc_str, l2_pol_str, l2_write_str,
                    l3_lines, l3_block, l3_assoc_str, l3_pol_str, l3_write_str
                );
            } else {
                cout << "Usage: init cache <l1_lines> <l1_block> <l1_assoc> <l1_pol> <l1_write>\n";
                cout << "                  <l2_lines> <l2_block> <l2_assoc> <l2_pol> <l2_write>\n";
                cout << "                  <l3_lines> <l3_block> <l3_assoc> <l3_pol> <l3_write>\n";
                cout << "Example: init cache 8 64 2way lru wt 16 64 2way lru wb 32 64 2way lru wb\n";
                cout << "  (use l3_lines=0 to skip L3)\n";
            }
        }
    }
    else if (cmd == "set") {
        string subcmd;
        iss >> subcmd;
        
        if (subcmd == "strategy") {
            string strat;
            iss >> strat;
            system.setAllocationStrategy(strat);
        }
        else if (subcmd == "vm_policy") {
            string policy;
            iss >> policy;
            system.setPageReplacementPolicy(policy);
        }
    }
    else if (cmd == "malloc") {
        size_t size;
        if (iss >> size) {
            system.allocate(size);
        } else {
            cout << "Usage: malloc <size>\n";
        }
    }
    else if (cmd == "free") {
        int id;
        if (iss >> id) {
            system.deallocate(id);
        } else {
            cout << "Usage: free <block_id>\n";
        }
    }
    else if (cmd == "read") {
        size_t addr;
        if (iss >> addr) {
            system.accessMemory(addr, false);  // false = read
        } else {
            cout << "Usage: read <address>\n";
        }
    }
    else if (cmd == "write") {
        size_t addr;
        if (iss >> addr) {
            system.accessMemory(addr, true);  // true = write
        } else {
            cout << "Usage: write <address>\n";
        }
    }
    else if (cmd == "access") {
        size_t addr;
        if (iss >> addr) {
            system.accessMemory(addr, false);  // Default to read
        } else {
            cout << "Usage: access <address>\n";
        }
    }
    else if (cmd == "dump") {
        system.displayMemoryLayout();
    }
    else if (cmd == "page_table") {
        system.displayPageTable();
    }
    else if (cmd == "cache_contents") {
        system.displayCacheContents();
    }
    else if (!cmd.empty()) {
        cout << "Unknown command. Type 'help' for available commands.\n";
    }
}

// ================================================================
// MAIN
// ================================================================

int main() {
    setupConsole();
    
    UnifiedMemorySystem system;
    printWelcome();
    
    string line;
    while (true) {
        cout << "> ";
        if (!getline(cin, line)) break;
        if (line.empty()) continue;
        if (line == "exit" || line == "quit") {
            cout << "\n========================================\n";
            cout << "Exiting Memory Management Simulator\n";
            cout << "Thank you for using the simulator!\n";
            cout << "========================================\n";
            break;
        }
        
        processCommand(system, line);
    }
    
    return 0;
}