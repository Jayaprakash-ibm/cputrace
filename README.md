# Ceph BlueStore Performance Profiling with `perfmon`

This document explains how to use the `perfmon` tool for hardware-level performance profiling and demonstrates its integration into the Ceph storage system, specifically for profiling the `BlueStore::_kv_sync_thread` function.

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

   - Obtain `perfmon.cc` and `perfmon.h` from the repository or your project.
   - Place them in your project directory (e.g., `src/`).

2. **Compile**

   Include `perfmon.cc` in your build system. For example, using `g++`:

   ```bash
   g++ -c perfmon.cc -o perfmon.o -std=c++11
   g++ main.cc perfmon.o -o my_program -pthread
   ```

## Example Usage in BlueStore

See the following commit for integration with BlueStore:

https://github.com/ceph/ceph/commit/c3d187fb25608b05cfc117e28b17be5dd082951e

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
Performance counter stats for '_kv_sync_thread (thread 0)':

         8,501 context-switches
   912,035,700 cycles
       394,552 cache-misses
     3,815,094 branch-misses
   828,148,000 instructions
```

