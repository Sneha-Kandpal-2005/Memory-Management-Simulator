#include <iostream>
#include <vector>
#include <map>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include "virtual_memory_simulator.h"

using namespace std;

// ==================== VIRTUAL MEMORY SIMULATOR ====================

VirtualMemorySimulator::VirtualMemorySimulator(size_t vm_size, size_t pm_size, size_t pg_size, string policy_str)
    : virtual_memory_size(vm_size),
        physical_memory_size(pm_size),
        page_size(pg_size),
        page_faults(0),
        page_hits(0),
        total_accesses(0),
        disk_reads(0),
        disk_writes(0),
        current_time(0),
        verbose(false) {
    
    // Calculate number of pages and frames
    num_virtual_pages = virtual_memory_size / page_size;
    num_physical_frames = physical_memory_size / page_size;
    
    // Validate configuration
    if (num_physical_frames > num_virtual_pages) {
        cout << "Warning: Physical memory larger than virtual memory!\n";
        num_physical_frames = num_virtual_pages;
        physical_memory_size = num_physical_frames * page_size;
    }
    
    // Initialize page table
    page_table.resize(num_virtual_pages);
    
    // Initialize frame tracking
    frame_to_page.resize(num_physical_frames, -1);
    frame_used.resize(num_physical_frames, false);
    
    // Set replacement policy
    if (policy_str == "lru") {
        policy = PageReplacementPolicy::LRU;
    } else {
        policy = PageReplacementPolicy::FIFO;
    }
    
    cout << "\n=== Virtual Memory Simulator Initialized ===\n";
    cout << "Virtual memory size: " << virtual_memory_size << " bytes\n";
    cout << "Physical memory size: " << physical_memory_size << " bytes\n";
    cout << "Page size: " << page_size << " bytes\n";
    cout << "Virtual pages: " << num_virtual_pages << "\n";
    cout << "Physical frames: " << num_physical_frames << "\n";
    cout << "Replacement policy: " << (policy == PageReplacementPolicy::FIFO ? "FIFO" : "LRU") << "\n";
    cout << "==========================================\n\n";
}

// Set replacement policy
void VirtualMemorySimulator::setReplacementPolicy(string policy_str) {
    if (policy_str == "lru") {
        policy = PageReplacementPolicy::LRU;
        cout << "Replacement policy set to: LRU\n";
    } else if (policy_str == "fifo") {
        policy = PageReplacementPolicy::FIFO;
        cout << "Replacement policy set to: FIFO\n";
    } else {
        cout << "Unknown policy. Available: fifo, lru\n";
    }
}

// Enable/disable verbose mode
void VirtualMemorySimulator::setVerbose(bool v) {
    verbose = v;
}

// Translate virtual address to physical address
size_t VirtualMemorySimulator::translateAddress(size_t virtual_address) {
    total_accesses++;
    current_time++;
    
    // Check if address is valid
    if (virtual_address >= virtual_memory_size) {
        cout << "ERROR: Virtual address 0x" << hex << virtual_address << dec
                << " exceeds virtual memory size!\n";
        return -1;
    }
    
    // Extract page number and offset
    int page_number = virtual_address / page_size;
    int offset = virtual_address % page_size;
    
    if (verbose) {
        cout << "\n--- Address Translation ---\n";
        cout << "Virtual address: 0x" << hex << virtual_address << dec << " (" << virtual_address << ")\n";
        cout << "Page number: " << page_number << "\n";
        cout << "Offset: " << offset << "\n";
    }
    
    // Check if page is in physical memory
    PageTableEntry& pte = page_table[page_number];
    
    if (pte.valid) {
        // PAGE HIT
        page_hits++;
        pte.last_access_time = current_time;
        pte.access_count++;
        
        // Calculate physical address
        size_t physical_address = (pte.frame_number * page_size) + offset;
        
        if (verbose) {
            cout << "Result: PAGE HIT\n";
            cout << "Frame number: " << pte.frame_number << "\n";
            cout << "Physical address: 0x" << hex << physical_address << dec << " (" << physical_address << ")\n";
        } else {
            cout << "Virtual 0x" << hex << virtual_address << " → Physical 0x" << physical_address << dec;
            cout << " [HIT]\n";
        }
        
        return physical_address;
    } else {
        // PAGE FAULT
        page_faults++;
        
        if (verbose) {
            cout << "Result: PAGE FAULT\n";
        } else {
            cout << "Virtual 0x" << hex << virtual_address << dec << " [FAULT] ";
        }
        
        // Handle page fault
        handlePageFault(page_number);
        
        // Now calculate physical address
        size_t physical_address = (pte.frame_number * page_size) + offset;
        
        if (!verbose) {
            cout << "→ Physical 0x" << hex << physical_address << dec << "\n";
        }
        
        return physical_address;
    }
}

// Handle page fault
void VirtualMemorySimulator::handlePageFault(int page_number) {
    if (verbose) {
        cout << "Handling page fault for page " << page_number << "...\n";
    }
    
    // Find free frame or select victim
    int free_frame = findFreeFrame();
    
    if (free_frame == -1) {
        // No free frame - must evict a page
        if (verbose) {
            cout << "No free frames. Selecting victim page...\n";
        }
        
        int victim_page = selectVictimPage();
        
        if (victim_page == -1) {
            cout << "ERROR: Could not find victim page!\n";
            return;
        }
        
        // Evict victim page
        free_frame = evictPage(victim_page);
    } else {
        if (verbose) {
            cout << "Found free frame: " << free_frame << "\n";
        }
    }
    
    // Load page into frame
    loadPage(page_number, free_frame);
}

// Find a free frame
int VirtualMemorySimulator::findFreeFrame() {
    for (int i = 0; i < num_physical_frames; i++) {
        if (!frame_used[i]) {
            return i;
        }
    }
    return -1;  // No free frame
}

// Select victim page using replacement policy
int VirtualMemorySimulator::selectVictimPage() {
    int victim = -1;
    
    if (policy == PageReplacementPolicy::FIFO) {
        // FIFO: Select page with earliest load time
        int min_load_time = INT32_MAX;
        
        for (int i = 0; i < num_physical_frames; i++) {
            int page_idx = frame_to_page[i];
            if (page_idx != -1 && page_table[page_idx].valid) {
                if (page_table[page_idx].load_time < min_load_time) {
                    min_load_time = page_table[page_idx].load_time;
                    victim = page_idx;
                }
            }
        }
        
        if (verbose && victim != -1) {
            cout << "FIFO selected victim: Page " << victim 
                    << " (load_time=" << page_table[victim].load_time << ")\n";
        }
        
    } else if (policy == PageReplacementPolicy::LRU) {
        // LRU: Select page with oldest access time
        int min_access_time = INT32_MAX;
        
        for (int i = 0; i < num_physical_frames; i++) {
            int page_idx = frame_to_page[i];
            if (page_idx != -1 && page_table[page_idx].valid) {
                if (page_table[page_idx].last_access_time < min_access_time) {
                    min_access_time = page_table[page_idx].last_access_time;
                    victim = page_idx;
                }
            }
        }
        
        if (verbose && victim != -1) {
            cout << "LRU selected victim: Page " << victim 
                    << " (last_access=" << page_table[victim].last_access_time << ")\n";
        }
    }
    
    return victim;
}

// Evict a page from memory
int VirtualMemorySimulator::evictPage(int page_number) {
    PageTableEntry& pte = page_table[page_number];
    
    if (!pte.valid) {
        cout << "ERROR: Trying to evict invalid page!\n";
        return -1;
    }
    
    int frame = pte.frame_number;
    
    if (verbose) {
        cout << "Evicting page " << page_number << " from frame " << frame;
        if (pte.dirty) {
            cout << " (dirty - writing to disk)";
            disk_writes++;
        }
        cout << "\n";
    }
    
    // If page is dirty, write to disk (simulated)
    if (pte.dirty) {
        disk_writes++;
    }
    
    // Invalidate page table entry
    pte.valid = false;
    pte.frame_number = -1;
    pte.dirty = false;
    
    // Mark frame as free
    frame_to_page[frame] = -1;
    frame_used[frame] = false;
    
    return frame;
}

// Load page into frame
void VirtualMemorySimulator::loadPage(int page_number, int frame_number) {
    PageTableEntry& pte = page_table[page_number];
    
    if (verbose) {
        cout << "Loading page " << page_number << " into frame " << frame_number << "\n";
    }
    
    // Simulate disk read
    disk_reads++;
    
    // Update page table entry
    pte.valid = true;
    pte.frame_number = frame_number;
    pte.dirty = false;
    pte.load_time = current_time;
    pte.last_access_time = current_time;
    pte.access_count++;
    
    // Update frame tracking
    frame_to_page[frame_number] = page_number;
    frame_used[frame_number] = true;
}

// Access a virtual address (simplified interface)
void VirtualMemorySimulator::access(size_t virtual_address) {
    translateAddress(virtual_address);
}

// Display page table
void VirtualMemorySimulator::displayPageTable() {
    cout << "\n=== PAGE TABLE ===\n";
    cout << "Format: Page | Valid | Frame | Dirty | Load_Time | Last_Access | Accesses\n\n";
    
    for (int i = 0; i < num_virtual_pages; i++) {
        PageTableEntry& pte = page_table[i];
        
        cout << "Page " << setw(3) << i << " | ";
        cout << (pte.valid ? "  YES " : "  NO  ") << " | ";
        
        if (pte.valid) {
            cout << setw(3) << pte.frame_number << "   | ";
            cout << (pte.dirty ? " YES " : " NO  ") << " | ";
            cout << setw(5) << pte.load_time << "     | ";
            cout << setw(6) << pte.last_access_time << "      | ";
            cout << setw(4) << pte.access_count;
        } else {
            cout << "  -   |   -   |     -     |      -      |    -    ";
        }
        
        cout << "\n";
    }
    
    cout << "\nPages in memory: ";
    int count = 0;
    for (int i = 0; i < num_virtual_pages; i++) {
        if (page_table[i].valid) {
            if (count > 0) cout << ", ";
            cout << i;
            count++;
        }
    }
    if (count == 0) cout << "None";
    cout << " (" << count << "/" << num_physical_frames << " frames used)\n";
}

// Display frame allocation
void VirtualMemorySimulator::displayFrames() {
    cout << "\n=== PHYSICAL FRAMES ===\n";
    cout << "Format: Frame | Page | Status\n\n";
    
    for (int i = 0; i < num_physical_frames; i++) {
        cout << "Frame " << setw(2) << i << " | ";
        
        if (frame_used[i]) {
            cout << "Page " << setw(2) << frame_to_page[i] << " | USED";
        } else {
            cout << "  -    | FREE";
        }
        
        cout << "\n";
    }
}

// Display statistics
void VirtualMemorySimulator::displayStats() {
    cout << "\n=== VIRTUAL MEMORY STATISTICS ===\n";
    
    cout << "\nConfiguration:\n";
    cout << "  Virtual memory: " << virtual_memory_size << " bytes (" << num_virtual_pages << " pages)\n";
    cout << "  Physical memory: " << physical_memory_size << " bytes (" << num_physical_frames << " frames)\n";
    cout << "  Page size: " << page_size << " bytes\n";
    cout << "  Replacement policy: " << (policy == PageReplacementPolicy::FIFO ? "FIFO" : "LRU") << "\n";
    
    cout << "\nMemory Access Statistics:\n";
    cout << "  Total accesses: " << total_accesses << "\n";
    cout << "  Page hits: " << page_hits << "\n";
    cout << "  Page faults: " << page_faults << "\n";
    
    if (total_accesses > 0) {
        double hit_rate = (double)page_hits / total_accesses * 100.0;
        double fault_rate = (double)page_faults / total_accesses * 100.0;
        
        cout << "  Hit rate: " << fixed << setprecision(2) << hit_rate << "%\n";
        cout << "  Fault rate: " << fixed << setprecision(2) << fault_rate << "%\n";
    }
    
    cout << "\nDisk Operations (Simulated):\n";
    cout << "  Disk reads: " << disk_reads << "\n";
    cout << "  Disk writes: " << disk_writes << "\n";
    cout << "  Total disk I/O: " << (disk_reads + disk_writes) << "\n";
    
    // Frame utilization
    int frames_used = 0;
    for (int i = 0; i < num_physical_frames; i++) {
        if (frame_used[i]) frames_used++;
    }
    
    double utilization = (double)frames_used / num_physical_frames * 100.0;
    
    cout << "\nFrame Utilization:\n";
    cout << "  Frames used: " << frames_used << " / " << num_physical_frames << "\n";
    cout << "  Utilization: " << fixed << setprecision(2) << utilization << "%\n";
}

// Clear all statistics
void VirtualMemorySimulator::clearStats() {
    page_faults = 0;
    page_hits = 0;
    total_accesses = 0;
    disk_reads = 0;
    disk_writes = 0;
    current_time = 0;
    
    cout << "Statistics cleared\n";
}

// Reset simulator
void VirtualMemorySimulator::reset() {
    // Clear page table
    for (int i = 0; i < num_virtual_pages; i++) {
        page_table[i] = PageTableEntry();
    }
    
    // Clear frame allocation
    for (int i = 0; i < num_physical_frames; i++) {
        frame_to_page[i] = -1;
        frame_used[i] = false;
    }
    
    // Clear statistics
    clearStats();
    
    cout << "Virtual memory simulator reset\n";
}