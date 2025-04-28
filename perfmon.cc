#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include "perfmon.h"

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

int HW_init(HW_counters *counters) {
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.size = sizeof(struct perf_event_attr);
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    pe.type = PERF_TYPE_HARDWARE;

    // cache misses counter
    pe.config = PERF_COUNT_HW_CACHE_MISSES;
    counters->fd_cache_misses = perf_event_open(&pe, 0, -1, -1, 0);
    if (counters->fd_cache_misses == -1) {
        perror("Error opening cache misses event");
        return -1;
    }

    // branch mispredictions counter.
    pe.config = PERF_COUNT_HW_BRANCH_MISSES;
    counters->fd_branch_misses = perf_event_open(&pe, 0, -1, -1, 0);
    if (counters->fd_branch_misses == -1) {
        perror("Error opening branch mispredictions event");
        close(counters->fd_cache_misses);
        return -1;
    }

    // total instructions counter.
    pe.config = PERF_COUNT_HW_INSTRUCTIONS;
    counters->fd_instructions = perf_event_open(&pe, 0, -1, -1, 0);
    if (counters->fd_instructions == -1) {
        perror("Error opening instructions event");
        close(counters->fd_cache_misses);
        close(counters->fd_branch_misses);
        return -1;
    }

    return 0;
}

int HW_start(HW_counters *counters) {
    // Reset and enable each counter individually.
    if (ioctl(counters->fd_cache_misses, PERF_EVENT_IOC_RESET, 0) == -1 ||
        ioctl(counters->fd_branch_misses, PERF_EVENT_IOC_RESET, 0) == -1 ||
        ioctl(counters->fd_instructions, PERF_EVENT_IOC_RESET, 0) == -1) {
        perror("Error resetting events");
        return -1;
    }

    if (ioctl(counters->fd_cache_misses, PERF_EVENT_IOC_ENABLE, 0) == -1 ||
        ioctl(counters->fd_branch_misses, PERF_EVENT_IOC_ENABLE, 0) == -1 ||
        ioctl(counters->fd_instructions, PERF_EVENT_IOC_ENABLE, 0) == -1) {
        perror("Error enabling events");
        return -1;
    }
    return 0;
}

int HW_stop(HW_counters *counters, HW_metrics *metrics) {
    // Disable each counter individually.
    if (ioctl(counters->fd_cache_misses, PERF_EVENT_IOC_DISABLE, 0) == -1 ||
        ioctl(counters->fd_branch_misses, PERF_EVENT_IOC_DISABLE, 0) == -1 ||
        ioctl(counters->fd_instructions, PERF_EVENT_IOC_DISABLE, 0) == -1) {
        perror("Error disabling events");
        return -1;
    }

    if (read(counters->fd_cache_misses, &metrics->cmiss, sizeof(long long)) == -1 ||
        read(counters->fd_branch_misses, &metrics->bmiss, sizeof(long long)) == -1 ||
        read(counters->fd_instructions, &metrics->ins, sizeof(long long)) == -1) {
        perror("Error reading events");
        return -1;
    }
    return 0;
}

int HW_cleanup(HW_counters *counters) {
    close(counters->fd_cache_misses);
    close(counters->fd_branch_misses);
    close(counters->fd_instructions);
    return 0;
}

