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

void HW_init(HW_ctx *ctx, HW_conf conf) {
    if (ctx->initialized) return;

    // Initialize context
    *ctx = {0};
    ctx->conf = conf;
    ctx->initialized = true;

    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.size = sizeof(struct perf_event_attr);
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    // Initialize counters based on configuration
    if (conf.capture_swi) {
        pe.type = PERF_TYPE_SOFTWARE;
        pe.config = PERF_COUNT_SW_CONTEXT_SWITCHES;
        ctx->fd_swi = perf_event_open(&pe, 0, -1, -1, 0);
        if (ctx->fd_swi == -1) {
            perror("Error opening context switches event");
        }
    }

    if (conf.capture_cyc) {
        pe.type = PERF_TYPE_HARDWARE;
        pe.config = PERF_COUNT_HW_CPU_CYCLES;
        ctx->fd_cyc = perf_event_open(&pe, 0, -1, -1, 0);
        if (ctx->fd_cyc == -1) {
            perror("Error opening cycles event");
        }
    }

    if (conf.capture_cmiss) {
        pe.type = PERF_TYPE_HARDWARE;
        pe.config = PERF_COUNT_HW_CACHE_MISSES;
        ctx->fd_cmiss = perf_event_open(&pe, 0, -1, -1, 0);
        if (ctx->fd_cmiss == -1) {
            perror("Error opening cache misses event");
        }
    }

    if (conf.capture_bmiss) {
        pe.type = PERF_TYPE_HARDWARE;
        pe.config = PERF_COUNT_HW_BRANCH_MISSES;
        ctx->fd_bmiss = perf_event_open(&pe, 0, -1, -1, 0);
        if (ctx->fd_bmiss == -1) {
            perror("Error opening branch mispredictions event");
        }
    }

    if (conf.capture_ins) {
        pe.type = PERF_TYPE_HARDWARE;
        pe.config = PERF_COUNT_HW_INSTRUCTIONS;
        ctx->fd_ins = perf_event_open(&pe, 0, -1, -1, 0);
        if (ctx->fd_ins == -1) {
            perror("Error opening instructions event");
        }
    }
}

void HW_start(HW_ctx *ctx) {
    if (ctx->conf.capture_swi && ctx->fd_swi != -1) {
        ioctl(ctx->fd_swi, PERF_EVENT_IOC_RESET, 0);
        ioctl(ctx->fd_swi, PERF_EVENT_IOC_ENABLE, 0);
    }
    if (ctx->conf.capture_cyc && ctx->fd_cyc != -1) {
        ioctl(ctx->fd_cyc, PERF_EVENT_IOC_RESET, 0);
        ioctl(ctx->fd_cyc, PERF_EVENT_IOC_ENABLE, 0);
    }
    if (ctx->conf.capture_cmiss && ctx->fd_cmiss != -1) {
        ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_RESET, 0);
        ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_ENABLE, 0);
    }
    if (ctx->conf.capture_bmiss && ctx->fd_bmiss != -1) {
        ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_RESET, 0);
        ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_ENABLE, 0);
    }
    if (ctx->conf.capture_ins && ctx->fd_ins != -1) {
        ioctl(ctx->fd_ins, PERF_EVENT_IOC_RESET, 0);
        ioctl(ctx->fd_ins, PERF_EVENT_IOC_ENABLE, 0);
    }
}


void HW_stop(HW_ctx *ctx, HW_metrics *metrics) {
    // Initialize metrics
    *metrics = {0};

    // Read current values and disable counters
    if (ctx->conf.capture_swi && ctx->fd_swi != -1) {
        read(ctx->fd_swi, &metrics->swi, sizeof(long long));
        ioctl(ctx->fd_swi, PERF_EVENT_IOC_DISABLE, 0);
    }
    if (ctx->conf.capture_cyc && ctx->fd_cyc != -1) {
        read(ctx->fd_cyc, &metrics->cyc, sizeof(long long));
        ioctl(ctx->fd_cyc, PERF_EVENT_IOC_DISABLE, 0);
    }
    if (ctx->conf.capture_cmiss && ctx->fd_cmiss != -1) {
        read(ctx->fd_cmiss, &metrics->cmiss, sizeof(long long));
        ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_DISABLE, 0);
    }
    if (ctx->conf.capture_bmiss && ctx->fd_bmiss != -1) {
        read(ctx->fd_bmiss, &metrics->bmiss, sizeof(long long));
        ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_DISABLE, 0);
    }
    if (ctx->conf.capture_ins && ctx->fd_ins != -1) {
        read(ctx->fd_ins, &metrics->ins, sizeof(long long));
        ioctl(ctx->fd_ins, PERF_EVENT_IOC_DISABLE, 0);
    }
}

void HW_clean(HW_ctx *ctx) {
    if (ctx->conf.capture_swi && ctx->fd_swi != -1) close(ctx->fd_swi);
    if (ctx->conf.capture_cyc && ctx->fd_cyc != -1) close(ctx->fd_cyc);
    if (ctx->conf.capture_cmiss && ctx->fd_cmiss != -1) close(ctx->fd_cmiss);
    if (ctx->conf.capture_bmiss && ctx->fd_bmiss != -1) close(ctx->fd_bmiss);
    if (ctx->conf.capture_ins && ctx->fd_ins != -1) close(ctx->fd_ins);
    ctx->initialized = false;
}
