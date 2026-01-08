# Expected Outputs for Test Workloads

## Test 1: Basic Allocation (`test1_basic_allocation.txt`)

**Components Tested:** Classic Allocator, First/Best/Worst Fit, Coalescing

**Expected Behavior:**
- **First Fit:** Allocates in first available space from beginning
- **Best Fit:** After freeing block 2 (200 bytes), allocates 180 bytes in that 200-byte hole (best match)
- **Worst Fit:** Allocates 50 bytes in largest available space
- **Coalescing:** When blocks 1 and 3 are freed, adjacent blocks merge into one large free block
- **Out of Memory:** 15000-byte allocation fails (exceeds 10000-byte memory)

**Key Metrics:**
- Allocation success rate: 85-90% (1 expected failure)
- External fragmentation: 10-30% before coalescing, <5% after coalescing
- All successful allocations return sequential block IDs

**Sample Output Lines:**
```
Allocated block id=1 at address=0x0000 (100 bytes)
Block 2 freed and merged
External fragmentation: 23.45%
Error: Not enough memory to allocate 15000 bytes
Success rate: 87.50%
```

---

## Test 2: Buddy System (`test2_buddy_system.txt`)

**Components Tested:** Buddy Allocator, Power-of-2 Rounding, Splitting, Merging

**Expected Behavior:**
- **Rounding:** 100→128, 50→64, 200→256, 32→32 (already power-of-2)
- **Internal Fragmentation:** 28 bytes (100→128), 14 bytes (50→64), 56 bytes (200→256)
- **Splitting:** When allocating 32 bytes, larger blocks split recursively
- **Merging:** After freeing blocks 1 and 2, buddies merge back up the tree
- **Non-power-of-2:** 1500 bytes adjusted to 2048 bytes automatically
- **Complete Allocation:** 4×512 = 2048 bytes fills entire memory
- **Out of Memory:** Final 64-byte allocation fails

**Key Metrics:**
- Internal fragmentation: 20-35% of allocated memory
- Split operations: 6-10
- Merge operations: 2-4
- Allocation success rate: 90-95%

**Sample Output Lines:**
```
Requested: 100 bytes
Rounded to: 128 bytes (order 3)
Internal fragmentation: 28 bytes (21.88%)
Block splits: 8
Block merges: 3
Warning: Buddy allocator requires power-of-2 size.
Adjusting to: 2048 bytes
```

---

## Test 3: Virtual Memory (`test3_virtual_memory.txt`)

**Components Tested:** Paging, Address Translation, FIFO/LRU Replacement

**Expected Behavior:**
- **First Accesses (1000, 1050, 1100):** All in same page → 1 fault, 2 hits
- **Page Boundaries (256, 512, 768):** Different pages → 3 faults
- **Frame Filling:** 64 reads fill all 64 frames (16384÷256)
- **Page Replacement:** Read at 20000 triggers FIFO replacement (evicts oldest)
- **LRU Advantage:** Accessing 1000 twice makes it recent, less likely to be evicted
- **Invalid Address:** 100000 exceeds 65536-byte virtual memory → error

**Key Metrics:**
- Initial page fault rate: 100% (cold start)
- After warm-up: Page fault rate 15-25%, hit rate 75-85%
- FIFO vs LRU: LRU typically 5-10% better hit rate
- Total page faults: 65-70 (64 to fill + 5-6 replacements)

**Sample Output Lines:**
```
Virtual 0x03E8 → Physical 0x0100 [FAULT] → Physical 0x0100
Virtual 0x041A → Physical 0x011A [HIT]
Page faults: 67
Page hits: 3
Hit rate: 4.29%
[After full workload]
Hit rate: 78.26%
ERROR: Virtual address 0x186A0 exceeds virtual memory size!
```

---

## Test 4: Cache Hierarchy (`test4_cache_hierarchy.txt`)

**Components Tested:** Multi-Level Cache, FIFO/LRU, Write Policies

**Expected Behavior:**
- **L1 Hits:** Repeated reads to address 100 → 1 miss, 2 hits
- **Block Alignment:** Addresses 0, 32, 63 in same 64-byte block → 1 miss, 2 hits
- **L2 Access:** After filling L1 (8 lines), subsequent accesses check L2
- **L3 Access:** After filling L2, L3 engaged
- **Memory Access:** Address 50000 likely misses all levels
- **Write-Back:** Multiple writes to same address → dirty bit set, no immediate memory write
- **Write-Back Eviction:** Filling cache with 5 writes (4 lines) → 1 dirty eviction
- **LRU Better:** Repeated access to 100 benefits from LRU vs FIFO

**Key Metrics:**
- L1 hit ratio: 50-70%
- L2 hit ratio: 20-35%
- L3 hit ratio: 10-20%
- Overall hit ratio: 65-85%
- Write-back policy: Memory writes < Total writes
- Write-through policy: Memory writes = Total writes

**Sample Output Lines:**
```
✓ L1 HIT (1 cycle)
✗ L1 MISS (+1 cycles) → checking L2...
✓ L2 HIT (10 cycles, total: 11 cycles)
→ Updated L1
L1: Hit ratio: 66.67%
L2: Hit ratio: 25.00%
Memory accesses: 5
Write-backs to memory: 1 (dirty evictions)
```

---

## Test 5: Write Policies (`test5_write_policies.txt`)

**Components Tested:** Write-Through, Write-Back, Dirty Bits, Write-Backs

**Expected Behavior:**
- **Write-Through:** All 5 writes → 5 memory writes (immediate propagation)
- **Write-Back:** 5 writes to 2 addresses → 2 cache misses, 3 cache hits, 0-1 memory writes initially
- **Write-Allocate:** First write to 100 → fetch block, then write (dirty)
- **Read After Write:** Read hits on dirty block (data in cache)
- **Dirty Eviction:** 5 writes to addresses 0, 64, 128, 192, 256 → fills 4-line cache → 1 dirty eviction
- **Mixed Eviction:** Clean reads + dirty writes → only dirty blocks cause write-backs
- **WT Immediate:** 3 writes → 3 memory writes (no caching of writes)

**Key Metrics:**
- Write-through: Memory writes = Total writes
- Write-back: Memory writes << Total writes (only on eviction)
- Write-back efficiency: 50-70% reduction in memory traffic
- Dirty evictions: 1-2 for small cache

**Sample Output Lines:**
```
[Write-Through]
Writes: 5
Memory writes: 5
[Write-Back]
Writes: 5
Memory writes: 1
Write-backs to memory: 1
[Dirty Block Eviction]
✗ L1 WRITE MISS → MEMORY READ (fetch block)
→ Updated L1 (dirty)
[On 5th write, eviction occurs]
Write-backs to memory: 1
```

---

## Test 6: Integrated System (`test6_integrated_system.txt`)

**Components Tested:** Full Integration (VM + Cache + Memory + Allocator)

**Expected Behavior:**
- **Verbose Read:** Shows complete flow:
  1. VM: Virtual 1000 → Physical translation → Page fault/hit
  2. Cache: Check L1→L2→L3 → Miss/Hit
  3. Memory: Final access or cache hit
- **Verbose Write:** Same flow, but write operation
- **Spatial Locality:** Addresses 3000-3020 (same page, same block) → 1 page fault, 1 cache miss, rest hits
- **Temporal Locality:** 4 reads to 4000 → 1 page fault, 1 cache miss, 3 cache hits
- **Mixed Operations:** Read/write to same address → hits after first access
- **Write-Back Integration:** Writes stay in cache (dirty), VM tracks pages
- **Working Set:** Repeated array access → high hit rates after warm-up
- **With Allocator:** malloc + read/write → all subsystems coordinate

**Key Metrics:**
- Overall hit ratio (VM + Cache): 60-80%
- Page fault rate: 10-20% (after warm-up)
- Cache hit rate: 70-85% (after warm-up)
- Spatial locality: 85-95% hit rate
- Temporal locality: 90-100% hit rate after first access

**Sample Output Lines:**
```
[STEP 1] VIRTUAL MEMORY - Address Translation
  Input: Virtual Address 0x3E8 (1000)
  ✓ Translation successful
  Output: Physical Address 0x100 (256)

[STEP 2] CACHE HIERARCHY - Multi-level Cache Check
  Operation: READ
  ✗ L1 MISS (+1 cycles) → checking L2...
  ✗ L2 MISS (+10 cycles) → checking L3...
  ✗ L3 MISS (+50 cycles) → accessing MEMORY

[STEP 3] PHYSICAL MEMORY - Final Access
  Reading from physical memory at 0x100
  ✓ Memory access complete

SUMMARY:
  Virtual Address: 0x3E8 (1000)
  Physical Address: 0x100 (256)
  Operation: READ
  Flow: VM Translation → Cache Hierarchy → Physical Memory
  Status: SUCCESS
```

---

## Test 7: Edge Cases (`test7_edge_cases.txt`)

**Components Tested:** Error Handling, Boundary Conditions, Recovery

**Expected Behavior:**
- **Zero Allocation:** malloc(0) → error
- **Oversized:** malloc(5000) on 1000-byte memory → error
- **Invalid Block:** free(9999) → "Block not found" error
- **Double Free:** Second free(1) → "Block not found" error
- **Invalid VM Address:** read(50000) on 32768-byte VM → "exceeds virtual memory" error
- **Direct Addressing:** Without VM/cache, reads work with direct physical addresses
- **Auto-Adjust:** 1500→2048 for buddy system
- **Cache Without VM:** Works correctly (cache on physical addresses)
- **Fragmentation Stress:** Creates high fragmentation, then recovers via coalescing
- **Buddy Stress:** Shows internal fragmentation accumulation

**Key Metrics:**
- Expected errors: 4-6
- Allocation success rate: 70-85% (stress scenarios include failures)
- Peak fragmentation: 50-70%
- After coalescing: <10% fragmentation
- All errors handled gracefully (no crashes)

**Sample Output Lines:**
```
Error: Cannot allocate 0 bytes
Error: Not enough memory to allocate 5000 bytes
Error: Invalid block_id 9999
Error: Block 1 not found
ERROR: Virtual address 0xC350 exceeds virtual memory size!
Warning: Buddy allocator requires power-of-2 size.
Adjusting to: 2048 bytes
[Before coalescing]
External fragmentation: 65.32%
[After coalescing]
External fragmentation: 4.17%
```

---

## General Success Criteria

### All Tests Pass If:
1. ✅ **No segmentation faults** or crashes
2. ✅ **Statistics in expected ranges** (±15% variation acceptable)
3. ✅ **Memory layout consistent** (no overlapping blocks, valid addresses)
4. ✅ **Error messages appear** for invalid operations
5. ✅ **Unique block IDs** (sequential, no duplicates)
6. ✅ **Coalescing works** (adjacent free blocks merge)
7. ✅ **Buddy merging correct** (buddies recombine)
8. ✅ **Page table valid** (no invalid frame numbers)
9. ✅ **Cache coherent** (no stale data issues)
10. ✅ **Write policies behave** as specified

---

## Verification Commands

### Quick Checks
```bash
# Run test
./memsim < tests/test1_basic_allocation.txt > tests/output1.txt

# Check for errors
grep -i "error" output1.txt | wc -l

# Check success rate
grep "Success rate" output1.txt

# Check hit rates
grep "Hit ratio" output1.txt
```
---
