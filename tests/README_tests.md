# Test Artifacts - Memory Management Simulator

## 📁 Files Included

```
tests/
├── test_workloads/                          # Demo Testing Workloads
│   ├── test1_basic_allocation.txt          # Classic allocator strategies
│   ├── test2_buddy_system.txt              # Buddy allocator operations  
│   ├── test3_virtual_memory.txt            # Paging and page replacement
│   ├── test4_cache_hierarchy.txt           # Multi-level cache testing
│   ├── test5_write_policies.txt            # Write-through vs write-back
│   ├── test6_integrated_system.txt         # Full system integration
│   ├── test7_edge_cases.txt                # Error handling & stress tests
│   └── test2_buddy_system.txt              # Buddy allocator operations 
│
├── test_outputs/
│   ├── output1.txt         
│   ├── output2.txt             
│   ├── output3.txt           
│   ├── output4.txt          
│   ├── output5.txt            
│   ├── output6.txt         
│   └── output7.txt               
│
├── EXPECTED_OUTPUTS.md                 # Detailed expected results
└── README_TESTS.md                     # This file
```
## 🚀 Quick Start

### Running Individual Tests
```bash
./memsim < tests/test_workloads/test1_basic_allocation.txt> tests/test_outputs/output1.txt
```

Output saved to `test_outputs/` directory.

## 📝 Detailed Test Descriptions

### Test 1: Basic Allocation
**What It Tests:**
- First Fit, Best Fit, Worst Fit allocation strategies
- Block splitting and coalescing
- External fragmentation tracking
- Out of memory handling

**Why This Test:**
- Foundation for understanding memory allocation
- Demonstrates strategy trade-offs
- Shows importance of coalescing

**Key Assertions:**
- ✓ All three strategies work correctly
- ✓ Coalescing merges adjacent free blocks
- ✓ Out of memory properly detected
- ✓ Fragmentation reduces after coalescing

---

### Test 2: Buddy System 
**What It Tests:**
- Power-of-2 allocation rounding
- Block splitting (recursive)
- Buddy merging (coalescing)
- Internal fragmentation
- Non-power-of-2 memory adjustment

**Why This Test:**
- Essential buddy system behavior
- Shows internal fragmentation trade-off
- Demonstrates splitting/merging algorithms

**Key Assertions:**
- ✓ Sizes rounded to power-of-2
- ✓ Internal fragmentation calculated correctly
- ✓ Buddies merge when both free
- ✓ Memory adjusted to power-of-2 automatically
- ✓ Split/merge counters increment

---

### Test 3: Virtual Memory 
**What It Tests:**
- Address translation (virtual → physical)
- Page faults and page hits
- Frame allocation
- FIFO page replacement
- LRU page replacement
- Page table management
- Invalid address handling

**Why This Test:**
- Core VM functionality
- Compares FIFO vs LRU policies
- Shows page replacement in action
- Longest test due to filling all frames

**Key Assertions:**
- ✓ First access causes page fault
- ✓ Repeated access causes page hit
- ✓ Page replacement triggers after frames full
- ✓ LRU performs better than FIFO
- ✓ Invalid addresses rejected
- ✓ Page table consistent

---

### Test 4: Cache Hierarchy 
**What It Tests:**
- L1, L2, L3 cache hits/misses
- Cache block alignment
- Set-associative mapping
- FIFO vs LRU replacement (cache)
- Write-through policy
- Write-back policy
- Cache evictions
- Direct-mapped vs associative

**Why This Test:**
- Complete cache hierarchy behavior
- Shows multi-level interaction
- Compares replacement policies
- Introduces write operations

**Key Assertions:**
- ✓ L1 hits on repeated access
- ✓ Same block addresses hit together
- ✓ L2/L3 engaged when L1 full
- ✓ LRU better than FIFO for temporal locality
- ✓ Write-back sets dirty bits
- ✓ Eviction causes write-backs (if dirty)

---

### Test 5: Write Policies 
**What It Tests:**
- Write-through vs write-back comparison
- Write-allocate policy
- Read after write
- Write after write
- Dirty block eviction
- Mixed clean/dirty evictions
- Memory write propagation

**Why This Test:**
- Critical write policy differences
- Shows performance trade-offs
- Demonstrates dirty bit tracking
- Validates write-back efficiency

**Key Assertions:**
- ✓ Write-through: every write → memory
- ✓ Write-back: writes stay in cache (dirty)
- ✓ Dirty blocks cause write-backs on eviction
- ✓ Clean blocks don't cause write-backs
- ✓ Write miss triggers write-allocate
- ✓ Memory writes: WT=total, WB<total

---

### Test 6: Integrated System 
**What It Tests:**
- Full VM + Cache + Memory flow
- Verbose mode (detailed flow)
- Spatial locality (same page/block)
- Temporal locality (repeated access)
- Mixed read/write operations
- Write-back in integrated system
- Working set patterns
- Memory allocation + access
- Buddy + VM + Cache together

**Why This Test:**
- **Most Important Test** - validates full integration
- Shows all subsystems working together
- Realistic usage patterns
- Demonstrates coordinated behavior

**Key Assertions:**
- ✓ Virtual → Physical → Cache → Memory flow
- ✓ Page faults affect cache misses
- ✓ Cache hits accelerate page hits
- ✓ Spatial locality improves both VM and cache
- ✓ Temporal locality shows high hit rates
- ✓ All statistics update correctly
- ✓ Memory allocator + VM + Cache coordinate

---

### Test 7: Edge Cases 
**What It Tests:**
- Zero-size allocation
- Oversized allocation
- Invalid block IDs
- Double free
- Invalid VM addresses
- Direct addressing (no VM/cache)
- Non-power-of-2 buddy adjustment
- Cache without VM
- Fragmentation stress
- Allocation failure and recovery

**Why This Test:**
- Validates error handling
- Tests boundary conditions
- Stress scenarios
- Recovery mechanisms

**Key Assertions:**
- ✓ All invalid operations rejected
- ✓ Error messages clear
- ✓ No crashes on invalid input
- ✓ System recovers from errors
- ✓ Fragmentation detected and handled
- ✓ Out of memory properly managed

---

## 📈 How to Interpret Results

### Expected Output Patterns

#### Test 1 - Classic Allocator:
```
✓ Strategy changes work
✓ Coalescing reduces fragmentation
✓ Out of memory detected
Success rate: 85-90%
External fragmentation: 5-30%
```

#### Test 2 - Buddy System:
```
✓ Sizes rounded up
✓ Internal fragmentation tracked
✓ Splits: 6-10, Merges: 2-4
✓ Auto-adjust to power-of-2
Internal fragmentation: 20-35%
```

#### Test 3 - Virtual Memory:
```
✓ Page faults initially, then hits
✓ LRU > FIFO hit rate
✓ Invalid addresses rejected
Page fault rate: 15-25%
Hit rate: 75-85%
```

#### Test 4 - Cache Hierarchy:
```
✓ L1 hit rate: 50-70%
✓ L2/L3 engaged when L1 full
✓ LRU better for temporal locality
Overall hit ratio: 65-85%
```

#### Test 5 - Write Policies:
```
✓ WT: Memory writes = Total writes
✓ WB: Memory writes < Total writes
✓ Dirty evictions cause write-backs
Write-back efficiency: 50-70%
```

#### Test 6 - Integration:
```
✓ All subsystems work together
✓ Verbose shows complete flow
✓ High hit rates with locality
Overall hit ratio: 70-85%
Page fault rate: 10-20%
```

#### Test 7 - Edge Cases:
```
✓ 4-6 expected errors
✓ All handled gracefully
✓ Peak fragmentation: 50-70%
✓ After coalescing: <10%
Success rate: 70-85%
```

## ✅ Acceptance Criteria

### All Tests Pass If:
- [ ] No segmentation faults
- [ ] Statistics within expected ranges (±15%)
- [ ] Memory layout consistent
- [ ] 4-6 expected errors in Test 7
- [ ] Coalescing reduces fragmentation
- [ ] LRU > FIFO performance
- [ ] Write-back < Write-through memory traffic
- [ ] Integration test shows complete flow


---