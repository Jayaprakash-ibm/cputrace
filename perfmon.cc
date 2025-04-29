#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <iomanip>
#include <sstream>
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

void print_perf_stats(const PerfSamples& samples) {
    printf("Function %s:\n", samples.func_name.c_str());

    auto compute_stats = [](const std::vector<long long>& values, long long& avg, double& stdev, long long& p99) {
        if (values.empty()) {
            avg = 0;
            stdev = 0.0;
            p99 = 0;
            return 0;
        }
        size_t n = values.size();

        long long sum = 0;
        for (long long v : values) sum += v;
        avg = n > 0 ? sum / n : 0;

        double variance = 0;
        for (long long v : values) {
            double diff = v - avg;
            variance += diff * diff;
        }
        variance = n > 0 ? variance / n : 0;
        stdev = std::sqrt(variance);

        std::vector<long long> sorted = values;
        std::sort(sorted.begin(), sorted.end());
        size_t p99_index = static_cast<size_t>(std::ceil(n * 0.99)) - 1;
        p99 = n > 0 ? sorted[p99_index] : 0;

        return static_cast<int>(n);
    };

    // Table formatting
    const int col1_width = 18; // Counter
    const int col2_width = 10; // Samples
    const int col3_width = 20; // Average
    const int col4_width = 18; // StdDev
    const int col5_width = 18; // P99

    // Print header
    printf("+%s+%s+%s+%s+%s+\n",
           std::string(col1_width, '-').c_str(),
           std::string(col2_width, '-').c_str(),
           std::string(col3_width, '-').c_str(),
           std::string(col4_width, '-').c_str(),
           std::string(col5_width, '-').c_str());
    printf("| %-*s | %-*s | %-*s | %-*s | %-*s |\n",
           col1_width, "Counter",
           col2_width, "Samples",
           col3_width, "Average",
           col4_width, "StdDev",
           col5_width, "P99");
    printf("+%s+%s+%s+%s+%s+\n",
           std::string(col1_width, '-').c_str(),
           std::string(col2_width, '-').c_str(),
           std::string(col3_width, '-').c_str(),
           std::string(col4_width, '-').c_str(),
           std::string(col5_width, '-').c_str());

    std::vector<long long> values;
    long long avg = 0;
    double stdev = 0.0;
    long long p99 = 0;

    if (samples.conf.capture_cyc) {
        values.clear();
        for (const auto& s : samples.samples) {
            values.push_back(s.cyc);
        }
        int n = compute_stats(values, avg, stdev, p99);
        std::ostringstream stdev_ss;
        stdev_ss << std::fixed << std::setprecision(6) << stdev;
        printf("| %-*s | %*d | %*lld | %*s | %*lld |\n",
               col1_width, "cycles",
               col2_width, n,
               col3_width, avg,
               col4_width, stdev_ss.str().c_str(),
               col5_width, p99);
    }

    if (samples.conf.capture_swi) {
        values.clear();
        avg = 0;
        stdev = 0.0;
        p99 = 0;
        for (const auto& s : samples.samples) {
            values.push_back(s.swi);
        }
        int n = compute_stats(values, avg, stdev, p99);
        std::ostringstream stdev_ss;
        stdev_ss << std::fixed << std::setprecision(6) << stdev;
        printf("| %-*s | %*d | %*lld | %*s | %*lld |\n",
               col1_width, "context switches",
               col2_width, n,
               col3_width, avg,
               col4_width, stdev_ss.str().c_str(),
               col5_width, p99);
    }

    if (samples.conf.capture_cmiss) {
        values.clear();
        avg = 0;
        stdev = 0.0;
        p99 = 0;
        for (const auto& s : samples.samples) {
            values.push_back(s.cmiss);
        }
        int n = compute_stats(values, avg, stdev, p99);
        std::ostringstream stdev_ss;
        stdev_ss << std::fixed << std::setprecision(6) << stdev;
        printf("| %-*s | %*d | %*lld | %*s | %*lld |\n",
               col1_width, "cache_misses",
               col2_width, n,
               col3_width, avg,
               col4_width, stdev_ss.str().c_str(),
               col5_width, p99);
    }

    if (samples.conf.capture_bmiss) {
        values.clear();
        avg = 0;
        stdev = 0.0;
        p99 = 0;
        for (const auto& s : samples.samples) {
            values.push_back(s.bmiss);
        }
        int n = compute_stats(values, avg, stdev, p99);
        std::ostringstream stdev_ss;
        stdev_ss << std::fixed << std::setprecision(6) << stdev;
        printf("| %-*s | %*d | %*lld | %*s | %*lld |\n",
               col1_width, "branch_misses",
               col2_width, n,
               col3_width, avg,
               col4_width, stdev_ss.str().c_str(),
               col5_width, p99);
    }

    if (samples.conf.capture_ins) {
        values.clear();
        avg = 0;
        stdev = 0.0;
        p99 = 0;
        for (const auto& s : samples.samples) {
            values.push_back(s.ins);
        }
        int n = compute_stats(values, avg, stdev, p99);
        std::ostringstream stdev_ss;
        stdev_ss << std::fixed << std::setprecision(6) << stdev;
        printf("| %-*s | %*d | %*lld | %*s | %*lld |\n",
               col1_width, "instructions",
               col2_width, n,
               col3_width, avg,
               col4_width, stdev_ss.str().c_str(),
               col5_width, p99);
    }

    // Print footer
    printf("+%s+%s+%s+%s+%s+\n",
           std::string(col1_width, '-').c_str(),
           std::string(col2_width, '-').c_str(),
           std::string(col3_width, '-').c_str(),
           std::string(col4_width, '-').c_str(),
           std::string(col5_width, '-').c_str());
}
