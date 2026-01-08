# Memory Management Simulator

A comprehensive operating system memory management simulator implementing dynamic memory allocation, multi-level caching, and virtual memory with paging.

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

## 📋 Table of Contents
- [Features](#features)
- [Architecture](#architecture)
- [Getting Started](#getting-started)
- [Usage](#usage)
- [Command Reference](#command-reference)
- [Examples](#examples)
- [Project Structure](#project-structure)
- [Key Feature Highlight](#key-features-highlight)
- [Design Observations](#design-observations)
- [Troubleshooting](#troubleshooting) 

## ✨ Features

### Memory Allocation
- **Classic Allocators**: First Fit, Best Fit, Worst Fit strategies with exact-size allocation
- **Buddy System**: Power-of-2 allocation with automatic splitting and buddy-merging/coalescing
- **Fragmentation Tracking**: Real-time reporting of internal (Buddy) and external (Classic) fragmentation metrics
- **Block Management**: Dynamic allocation/deallocation with automatic coalescing

### Multi-Level Cache Hierarchy
- **3-Level Cache**: Configurable L1, L2, and L3 level caches with custom block sizes and associativity
- **Associativity**: Direct-mapped, 2-way, 4-way, and fully associative
- **Replacement Policies**: FIFO and LRU algorithms
- **Write Management**: Supports **Write-Through** and **Write-Back** with dirty-bit tracking.
- **Write-Allocate**: Automatically fetches blocks into cache on write-misses to improve temporal locality.
- **Performance Metrics**: Hit/miss ratios, average access time, write-back tracking

### Virtual Memory
- **Paging System**: Configurable page size and replacement policies (FIFO/LRU)
- **Address Translation**: Virtual → Physical address mapping
- **Page Replacement**: FIFO and LRU algorithms
- **Page Fault Handling**: Automatic page loading and victim selection
- **Statistics**: Page fault rate, hit rate, disk I/O simulation (disk reads and disk writes)

### Unified Integration
- **Automatic Flow**: Virtual Address → Page Table → Physical Address → Cache → Memory
- **Modular Design**: Enable/disable components independently
- **Interactive CLI**: User-friendly command interface with setup wizard
- **Comprehensive Statistics**: Detailed metrics from all subsystems

## 🏗️ Architecture

```
┌─────────────────┐
│ Virtual Address │
└────────┬────────┘
         │
         v
┌─────────────────┐
│   Page Table    │ ← Virtual Memory (if enabled)
│  (Translation)  │
└────────┬────────┘
         │
         v
┌─────────────────┐
│Physical Address │
└────────┬────────┘
         │
         v
┌─────────────────┐
│  Cache Levels   │ ← L1 → L2 → L3 (if enabled)
│  (Read/Write)   │
└────────┬────────┘
         │
         v
┌─────────────────┐
│Physical Memory  │ ← Classic or Buddy Allocator
└─────────────────┘
```
**Note:** The `UnifiedMemorySystem` class acts as a Facade, ensuring that regardless of whether VM or Cache is enabled, the request always follows the correct hardware-logic sequence, preventing 'illegal' direct-to-memory shortcuts.

## 🚀 Getting Started

### Prerequisites
- C++17 compatible compiler (g++, clang++, MSVC)
- **Windows Users**: To use `make`, install **MinGW-w64** (provides `mingw32-make`).

### Building the Project

#### Option 1: Using the Cross-Platform Makefile (Recommended)
The Makefile automatically detects your OS to manage build paths and extensions.
```bash
make       # Build the simulator
make run   # Build and launch
make clean # Remove build artifacts
```

#### Option 2: Manual Compilation - Using g++ directly 

```bash
cd src
g++ -std=c++17 -I../include -o memsim.exe main.cpp allocator/memory_allocator.cpp buddy/buddy_allocator.cpp cache/cache_simulator.cpp virtual_memory/virtual_memory_simulator.cpp
./memsim
```

### Quick Start

```bash
# Start the simulator
./integrated

# Initialize memory (1024 bytes, classic allocator)
> init memory 1024

# Set up cache interactively
> setup cache

# Start using the system
> malloc 100
> write 500
> stats
```

## 📖 Usage

### Basic Workflow

1. **Initialize Physical Memory** (Required)
   ```
   > init memory 4096
   ```

2. **Enable Optional Components**
   - Virtual Memory: `init vm 16384 256 lru`
   - Cache Hierarchy: `setup cache` (interactive wizard)

3. **Perform Operations**
   - Allocate: `malloc 200`
   - Access: `read 1000` or `write 1000`
   - Free: `free 1`

4. **View Statistics**
   ```
   > stats
   > dump
   > cache_contents
   > page_table
   ```

## 📚 Command Reference

### System Initialization
| Command | Description | Example |
|---------|-------------|---------|
| `init memory <size> [buddy]` | Initialize memory allocator | `init memory 1024 buddy` |
| `init vm <vm_size> <page_size> [policy]` | Enable virtual memory | `init vm 65536 256 lru` |
| `setup cache` | Interactive cache setup wizard | `setup cache` |

### Memory Operations
| Command | Description | Example |
|---------|-------------|---------|
| `malloc <size>` | Allocate memory | `malloc 500` |
| `free <block_id>` | Deallocate memory | `free 1` |
| `read <address>` | Read from address | `read 1000` |
| `write <address>` | Write to address | `write 2000` |

### Configuration
| Command | Description | Example |
|---------|-------------|---------|
| `set strategy <type>` | Set allocation strategy | `set strategy best_fit` |
| `set vm_policy <policy>` | Set page replacement | `set vm_policy lru` |
| `verbose <on\|off>` | Toggle detailed output | `verbose on` |

### Information & Statistics
| Command | Description |
|---------|-------------|
| `status` | Show system configuration |
| `stats` | Display comprehensive statistics |
| `dump` | Show memory layout |
| `page_table` | Display page table (if VM enabled) |
| `cache_contents` | Show cache contents |

### System Control
| Command | Description |
|---------|-------------|
| `clear` | Reset entire system |
| `help` | Show command reference |
| `exit` | Exit simulator |

## 💡 Examples

### Example 1: Classic Allocator with Fragmentation

```bash
> init memory 1000
> set strategy first_fit
> malloc 200
> malloc 300
> malloc 150
> free 2
> dump
# Shows fragmentation - 300-byte hole in middle
> set strategy best_fit
> malloc 250
> dump
# Shows best-fit placed 250 bytes in 300-byte hole
```

### Example 2: Buddy System

```bash
> init memory 1024 buddy
> malloc 100
# Allocated 128 bytes (rounded to power of 2)
# Internal fragmentation: 28 bytes
> malloc 200
# Allocated 256 bytes
> dump
# Shows buddy blocks
> free 1
> free 2
> dump
# Shows automatic buddy merging
```

### Example 3: Write Policies Comparison

```bash
# Write-Through
> init memory 4096
> setup cache
# Configure: L1 with write-through
> write 1000
> write 1000
> write 1000
> stats
# Memory writes = 3 (all propagate immediately)

> clear

# Write-Back
> init memory 4096
> setup cache
# Configure: L1 with write-back
> write 1000
> write 1000
> write 1000
> stats
# Memory writes = 0 (deferred until eviction)
# Write-backs = 0 (no eviction yet)
```

### Example 4: Full System Integration

```bash
> init memory 16384 buddy
> init vm 65536 256 lru
> setup cache
# Configure 3-level cache with write-back

> malloc 1000
> verbose on
> write 5000
# Shows complete flow:
# Virtual → Physical translation
# Cache hierarchy check (L1 → L2 → L3)
# Memory access

> stats
# Shows statistics from all subsystems
```

## 📁 Project Structure

```
memory-management-simulator/
├── include/
│   ├── memory_allocator.h       # Classic allocator interface
│   ├── buddy_allocator.h        # Buddy system interface
│   ├── cache_simulator.h        # Cache hierarchy interface
│   └── virtual_memory_simulator.h # Virtual memory interface
│
├── src/
│   ├── main.cpp                 # Unified integration and CLI
│   ├── allocator/
│   │   └── memory_allocator.cpp # First/Best/Worst Fit implementation
│   ├── buddy/
│   │   └── buddy_allocator.cpp  # Buddy system implementation
│   ├── cache/
│   │   └── cache_simulator.cpp  # Multi-level cache implementation
│   └── virtual_memory/
│       └── virtual_memory_simulator.cpp # Paging implementation
│
├── docs/
│   └── Design Document.pdf      # Design document
│
├── tests/
│   ├── test_workloads/          # Quick demo test cases
│   ├── test_outputs/            # Logs of demo test cases
│   ├── EXPECTED_OUTPUTS.md      # A compilation of what is expected from different demo tests
│   └── README_tests.md          # A guide on how to run test cases
│
├── Makefile                     # Build configuration
└── README.md                    # This file
```

## 🎯 Key Features Highlight

### Write Policy Support
The cache simulator implements both write-through and write-back policies:
- **Write-Through**: Every write propagates to memory immediately (simple, consistent)
- **Write-Back**: Writes stay in cache until eviction (faster, lower memory traffic)
- **Dirty Bit Tracking**: Automatically tracks modified cache blocks
- **Write-Back Counter**: Monitors dirty block evictions

### Intelligent Allocation
- **Coalescing**: Automatically merges adjacent free blocks
- **Splitting**: Divides large blocks to satisfy small requests
- **Buddy Merging**: Recursive buddy coalescing in buddy system
- **Fragmentation Metrics**: Real-time internal and external fragmentation tracking

### Interactive Setup
- **Cache Configuration Wizard**: Step-by-step interactive cache setup
- **Sensible Defaults**: Pre-filled default values for quick configuration
- **Validation**: Automatic validation of configuration parameters
- **Summary Display**: Clear summary before applying configuration

## 💡 Design Observations
- **Exact Allocation**: The Classic Allocator eliminates internal fragmentation by splitting blocks to match requested sizes exactly.
- **Deferred Updates**: In Write-Back mode, memory writes are only triggered when a modified (dirty) block is evicted.
- **Facade Pattern**: The UnifiedMemorySystem ensures all accesses follow hardware-correct sequences, preventing direct-to-memory shortcuts.

## 🐛 Troubleshooting

### Common Issues

**1. Compilation errors with g++**
- Ensure C++17 support: `g++ --version` (should be 7.0+)
- Use `-std=c++17` flag explicitly

**2. Makefile issues**
- Path separators differ between Windows (`\`) and Linux (`/`)
- Use Option 2 (direct g++ compilation) for unresolved issues

**3. "Command not found" errors**
- Make sure you're in the `src/` directory
- Use `./memsim`


## 📝 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 👥 Authors

- **Sneha Kandpal** 

## 🙏 Acknowledgments

- Operating System Concepts (Silberschatz, Galvin, Gagne)
- Modern Operating Systems (Andrew Tanenbaum)
- Computer Architecture (Patterson & Hennessy)
