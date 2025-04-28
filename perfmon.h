#ifndef PERFMON_H
#define PERFMON_H

#include <stdint.h>

// measured hardware events.
typedef struct {
    long long cmiss;
    long long bmiss;
    long long ins;
} HW_metrics;

// file descriptors for the counters.
typedef struct {
    int fd_cache_misses;
    int fd_branch_misses;
    int fd_instructions;
} HW_counters;


int HW_init(HW_counters *counters);

int HW_start(HW_counters *counters);

int HW_stop(HW_counters *counters, HW_metrics *metrics);

int HW_cleanup(HW_counters *counters);

#endif // PERFMON_H

