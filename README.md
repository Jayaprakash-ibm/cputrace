# Ceph BlueStore Performance Profiling with `cputrace`

This document explains how to use the `cputrace` tool for hardware-level performance profiling and demonstrates its integration into the Ceph storage system, specifically for profiling the `BlueStore::_kv_sync_thread` function.

## Features

The tool collects the following metrics using Linux performance counters:

- **Context Switches**  
  Software context switches  
  (`PERF_COUNT_SW_CONTEXT_SWITCHES`)

- **CPU Cycles**  
  Total CPU cycles consumed  
  (`PERF_COUNT_HW_CPU_CYCLES`)

- **Cache Misses**  
  Hardware cache misses  
  (`PERF_COUNT_HW_CACHE_MISSES`)

- **Branch Misses**  
  Branch prediction misses  
  (`PERF_COUNT_HW_BRANCH_MISSES`)

- **Instructions**  
  Total instructions executed  
  (`PERF_COUNT_HW_INSTRUCTIONS`)

## Installation

### Standalone Usage

1. **Copy Source Files**

   - Obtain `cputrace.cc` and `cputrace.h` from the repository or your project.
   - Place them in your project directory (e.g., `src/`).

2. **Compile**

   Include `cputrace.cc` in your build system. For example, using `g++`:

   ```bash
   g++ -c cputrace.cc -o cputrace.o -std=c++11
   g++ main.cc cputrace.o -o my_program -pthread
   ```

## Example Usage in BlueStore

See the following commit for integration with BlueStore:

[commit](https://github.com/ceph/ceph/commit/2f1135b98afdba2034175c6f25a22187d99a2ac0)

To run the test:

```bash
./bin/ceph_test_objectstore \
  --gtest_filter="ObjectStore/StoreTest.Synthetic/1" \
  --log_to_stderr=false \
  --log_file=log.log \
  --debug_bluestore=20
```

## Output

Sample output from profiling `BlueStore::_kv_sync_thread`:

```
Performance counter stats for '_kv_sync_thread (thread 0)' (6 calls):

          13,357 context-switches
         2,226.2 avg context-switches
   1,396,201,041 cycles
   232,700,173.5 avg cycles
         290,241 cache-misses
        48,373.5 avg cache-misses
       6,496,508 branch-misses
     1,082,751.3 avg branch-misses
   1,233,944,675 instructions
   205,657,445.8 avg instructions
```

