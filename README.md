# Perfmon: Hardware Performance Monitoring Library

A small, non-portable (x86-64) library for measuring hardware performance counters using Linux `perf_event_open`. Designed for profiling Ceph's BlueStore.

## Files
- `perfmon.h`, `perfmon.cc`: Library implementation.
- `test1.cc`: Test for cycles and context switches.
- `test2.cc`: Test for cache misses, branch mispredictions, instructions.

