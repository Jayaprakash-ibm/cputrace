#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <errno.h>
#include <pthread.h>
#include <inttypes.h>
#include "cputrace.h"

// Global profiler instance
static struct cputrace_profiler g_profiler;


static void initialize_profiler() {
    struct Arena* profiler_arena = arena_create(sizeof(struct cputrace_anchor) * CPUTRACE_MAX_ANCHORS, true);
    if (!profiler_arena) {
        fprintf(stderr, "initialize_profiler: Error: arena_create failed\n");
        exit(1);
    }
    g_profiler.anchors = (struct cputrace_anchor*)arena_alloc(profiler_arena, sizeof(struct cputrace_anchor) * CPUTRACE_MAX_ANCHORS);
    if (!g_profiler.anchors) {
        fprintf(stderr, "initialize_profiler: Error: arena_alloc anchors failed\n");
        arena_destroy(profiler_arena);
        exit(1);
    }
    memset(g_profiler.anchors, 0, sizeof(struct cputrace_anchor) * CPUTRACE_MAX_ANCHORS);
    for (uint64_t i = 0; i < CPUTRACE_MAX_ANCHORS; i++) {
        pthread_mutex_init(&g_profiler.anchors[i].mutex, NULL);
        g_profiler.anchors[i].results_arena = arena_create(20 * 1024 * 1024, false);
        if (!g_profiler.anchors[i].results_arena) {
            fprintf(stderr, "initialize_profiler: Error: arena_create for anchor %lu failed\n", i);
            for (uint64_t j = 0; j < i; j++) {
                if (g_profiler.anchors[j].results_arena) {
                    arena_destroy(g_profiler.anchors[j].results_arena);
                }
                pthread_mutex_destroy(&g_profiler.anchors[j].mutex);
            }
            arena_destroy(profiler_arena);
            exit(1);
        }
    }
    g_profiler.profiling = true;
    pthread_mutex_init(&g_profiler.file_mutex, NULL);
    if (atexit(cputrace_close) != 0) {
        fprintf(stderr, "initialize_profiler: Error: atexit registration failed\n");
        cputrace_close();
        exit(1);
    }
}

// Static variable to trigger initialization
static struct ProfilerInitializer {
    ProfilerInitializer() { initialize_profiler(); }
} profiler_initializer;

static long perf_event_open(struct perf_event_attr* hw_event, pid_t pid,
                           int cpu, int group_fd, unsigned long flags) {
    long fd = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
    if (fd == -1) {
        fprintf(stderr, "%s: failed: %s (errno=%d)\n", __func__, strerror(errno), errno);
    }
    return fd;
}

void HW_init(struct HW_ctx* ctx, struct HW_conf* conf) {
    ctx->fd_swi = -1;
    ctx->fd_cyc = -1;
    ctx->fd_cmiss = -1;
    ctx->fd_bmiss = -1;
    ctx->fd_ins = -1;
    ctx->conf = *conf;
}

void HW_start(struct HW_ctx* ctx) {
    struct perf_event_attr pe;

    if (ctx->conf.capture_swi) {
        if (ctx->fd_swi == -1) {
            memset(&pe, 0, sizeof(pe));
            pe.size = sizeof(pe);
            pe.type = PERF_TYPE_SOFTWARE;
            pe.config = PERF_COUNT_SW_CONTEXT_SWITCHES;
            pe.disabled = 1;
            pe.inherit = 1;
            ctx->fd_swi = perf_event_open(&pe, 0, -1, -1, 0);
            if (ctx->fd_swi != -1) {
                if (ioctl(ctx->fd_swi, PERF_EVENT_IOC_RESET, 0) == -1) {
                    fprintf(stderr, "%s: ioctl RESET failed for swi: %s\n", __func__, strerror(errno));
                }
                if (ioctl(ctx->fd_swi, PERF_EVENT_IOC_ENABLE, 0) == -1) {
                    fprintf(stderr, "%s: ioctl ENABLE failed for swi: %s\n", __func__, strerror(errno));
                }
            } else {
                ctx->conf.capture_swi = false;
                fprintf(stderr, "%s: Failed to open swi counter\n", __func__);
            }
        } else {
            if (ioctl(ctx->fd_swi, PERF_EVENT_IOC_RESET, 0) == -1) {
                fprintf(stderr, "%s: ioctl RESET failed for swi: %s\n", __func__, strerror(errno));
            }
        }
    }

    if (ctx->conf.capture_cyc) {
        if (ctx->fd_cyc == -1) {
            memset(&pe, 0, sizeof(pe));
            pe.size = sizeof(pe);
            pe.type = PERF_TYPE_HARDWARE;
            pe.config = PERF_COUNT_HW_CPU_CYCLES;
            pe.disabled = 1;
            pe.inherit = 1;
            ctx->fd_cyc = perf_event_open(&pe, 0, -1, -1, 0);
            if (ctx->fd_cyc != -1) {
                if (ioctl(ctx->fd_cyc, PERF_EVENT_IOC_RESET, 0) == -1) {
                    fprintf(stderr, "%s: ioctl RESET failed for cyc: %s\n", __func__, strerror(errno));
                }
                if (ioctl(ctx->fd_cyc, PERF_EVENT_IOC_ENABLE, 0) == -1) {
                    fprintf(stderr, "%s: ioctl ENABLE failed for cyc: %s\n", __func__, strerror(errno));
                }
            } else {
                ctx->conf.capture_cyc = false;
                fprintf(stderr, "%s: Failed to open cyc counter\n", __func__);
            }
        } else {
            if (ioctl(ctx->fd_cyc, PERF_EVENT_IOC_RESET, 0) == -1) {
                fprintf(stderr, "%s: ioctl RESET failed for cyc: %s\n", __func__, strerror(errno));
            }
        }
    }

    if (ctx->conf.capture_cmiss) {
        if (ctx->fd_cmiss == -1) {
            memset(&pe, 0, sizeof(pe));
            pe.size = sizeof(pe);
            pe.type = PERF_TYPE_HARDWARE;
            pe.config = PERF_COUNT_HW_CACHE_MISSES;
            pe.disabled = 1;
            pe.inherit = 1;
            ctx->fd_cmiss = perf_event_open(&pe, 0, -1, -1, 0);
            if (ctx->fd_cmiss != -1) {
                if (ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_RESET, 0) == -1) {
                    fprintf(stderr, "%s: ioctl RESET failed for cmiss: %s\n", __func__, strerror(errno));
                }
                if (ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_ENABLE, 0) == -1) {
                    fprintf(stderr, "%s: ioctl ENABLE failed for cmiss: %s\n", __func__, strerror(errno));
                }
            } else {
                ctx->conf.capture_cmiss = false;
                fprintf(stderr, "%s: Failed to open cmiss counter\n", __func__);
            }
        } else {
            if (ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_RESET, 0) == -1) {
                fprintf(stderr, "%s: ioctl RESET failed for cmiss: %s\n", __func__, strerror(errno));
            }
        }
    }

    if (ctx->conf.capture_bmiss) {
        if (ctx->fd_bmiss == -1) {
            memset(&pe, 0, sizeof(pe));
            pe.size = sizeof(pe);
            pe.type = PERF_TYPE_HARDWARE;
            pe.config = PERF_COUNT_HW_BRANCH_MISSES;
            pe.disabled = 1;
            pe.inherit = 1;
            ctx->fd_bmiss = perf_event_open(&pe, 0, -1, -1, 0);
            if (ctx->fd_bmiss != -1) {
                if (ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_RESET, 0) == -1) {
                    fprintf(stderr, "%s: ioctl RESET failed for bmiss: %s\n", __func__, strerror(errno));
                }
                if (ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_ENABLE, 0) == -1) {
                    fprintf(stderr, "%s: ioctl ENABLE failed for bmiss: %s\n", __func__, strerror(errno));
                }
            } else {
                ctx->conf.capture_bmiss = false;
                fprintf(stderr, "%s: Failed to open bmiss counter\n", __func__);
            }
        } else {
            if (ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_RESET, 0) == -1) {
                fprintf(stderr, "%s: ioctl RESET failed for bmiss: %s\n", __func__, strerror(errno));
            }
        }
    }

    if (ctx->conf.capture_ins) {
        if (ctx->fd_ins == -1) {
            memset(&pe, 0, sizeof(pe));
            pe.size = sizeof(pe);
            pe.type = PERF_TYPE_HARDWARE;
            pe.config = PERF_COUNT_HW_INSTRUCTIONS;
            pe.disabled = 1;
            pe.inherit = 1;
            ctx->fd_ins = perf_event_open(&pe, 0, -1, -1, 0);
            if (ctx->fd_ins != -1) {
                if (ioctl(ctx->fd_ins, PERF_EVENT_IOC_RESET, 0) == -1) {
                    fprintf(stderr, "%s: ioctl RESET failed for ins: %s\n", __func__, strerror(errno));
                }
                if (ioctl(ctx->fd_ins, PERF_EVENT_IOC_ENABLE, 0) == -1) {
                    fprintf(stderr, "%s: ioctl ENABLE failed for ins: %s\n", __func__, strerror(errno));
                }
            } else {
                ctx->conf.capture_ins = false;
                fprintf(stderr, "%s: Failed to open ins counter\n", __func__);
            }
        } else {
            if (ioctl(ctx->fd_ins, PERF_EVENT_IOC_RESET, 0) == -1) {
                fprintf(stderr, "%s: ioctl RESET failed for ins: %s\n", __func__, strerror(errno));
            }
        }
    }
}

void HW_stop(struct HW_ctx* ctx, struct HW_measure* measure) {
    if (ctx->conf.capture_swi && ctx->fd_swi != -1) {
        if (ioctl(ctx->fd_swi, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            fprintf(stderr, "%s: ioctl DISABLE failed for swi: %s\n", __func__, strerror(errno));
        }
        long long value;
        if (read(ctx->fd_swi, &value, sizeof(long long)) == -1) {
            fprintf(stderr, "%s: read final failed for swi: %s\n", __func__, strerror(errno));
            measure->swi = 0;
        } else {
            measure->swi = value;
        }
    }
    if (ctx->conf.capture_cyc && ctx->fd_cyc != -1) {
        if (ioctl(ctx->fd_cyc, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            fprintf(stderr, "%s: ioctl DISABLE failed for cyc: %s\n", __func__, strerror(errno));
        }
        long long value;
        if (read(ctx->fd_cyc, &value, sizeof(long long)) == -1) {
            fprintf(stderr, "%s: read final failed for cyc: %s\n", __func__, strerror(errno));
            measure->cyc = 0;
        } else {
            measure->cyc = value;
        }
    }
    if (ctx->conf.capture_cmiss && ctx->fd_cmiss != -1) {
        if (ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            fprintf(stderr, "%s: ioctl DISABLE failed for cmiss: %s\n", __func__, strerror(errno));
        }
        long long value;
        if (read(ctx->fd_cmiss, &value, sizeof(long long)) == -1) {
            fprintf(stderr, "%s: read final failed for cmiss: %s\n", __func__, strerror(errno));
            measure->cmiss = 0;
        } else {
            measure->cmiss = value;
        }
    }
    if (ctx->conf.capture_bmiss && ctx->fd_bmiss != -1) {
        if (ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            fprintf(stderr, "%s: ioctl DISABLE failed for bmiss: %s\n", __func__, strerror(errno));
        }
        long long value;
        if (read(ctx->fd_bmiss, &value, sizeof(long long)) == -1) {
            fprintf(stderr, "%s: read final failed for bmiss: %s\n", __func__, strerror(errno));
            measure->bmiss = 0;
        } else {
            measure->bmiss = value;
        }
    }
    if (ctx->conf.capture_ins && ctx->fd_ins != -1) {
        if (ioctl(ctx->fd_ins, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            fprintf(stderr, "%s: ioctl DISABLE failed for ins: %s\n", __func__, strerror(errno));
        }
        long long value;
        if (read(ctx->fd_ins, &value, sizeof(long long)) == -1) {
            fprintf(stderr, "%s: read final failed for ins: %s\n", __func__, strerror(errno));
            measure->ins = 0;
        } else {
            measure->ins = value;
        }
    }
}

void HW_clean(struct HW_ctx* ctx) {
    if (ctx->conf.capture_swi && ctx->fd_swi != -1) {
        ioctl(ctx->fd_swi, PERF_EVENT_IOC_DISABLE, 0);
        close(ctx->fd_swi);
        ctx->fd_swi = -1;
    }
    if (ctx->conf.capture_cyc && ctx->fd_cyc != -1) {
        ioctl(ctx->fd_cyc, PERF_EVENT_IOC_DISABLE, 0);
        close(ctx->fd_cyc);
        ctx->fd_cyc = -1;
    }
    if (ctx->conf.capture_cmiss && ctx->fd_cmiss != -1) {
        ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_DISABLE, 0);
        close(ctx->fd_cmiss);
        ctx->fd_cmiss = -1;
    }
    if (ctx->conf.capture_bmiss && ctx->fd_bmiss != -1) {
        ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_DISABLE, 0);
        close(ctx->fd_bmiss);
        ctx->fd_bmiss = -1;
    }
    if (ctx->conf.capture_ins && ctx->fd_ins != -1) {
        ioctl(ctx->fd_ins, PERF_EVENT_IOC_DISABLE, 0);
        close(ctx->fd_ins);
        ctx->fd_ins = -1;
    }
}

static struct ArenaRegion* arena_region_create(size_t size) {
    void* start = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (start == MAP_FAILED) {
        fprintf(stderr, "%s: mmap failed: %s\n", __func__, strerror(errno));
        return NULL;
    }
    struct ArenaRegion* region = (struct ArenaRegion*)malloc(sizeof(struct ArenaRegion));
    region->start = start;
    region->end = (void*)((uintptr_t)start + size);
    region->current = start;
    region->next = NULL;
    return region;
}

static void arena_region_destroy(struct ArenaRegion* region) {
    munmap(region->start, (uintptr_t)region->end - (uintptr_t)region->start);
    free(region);
}

static uint64_t arena_region_size(struct ArenaRegion* region) {
    return (uintptr_t)region->end - (uintptr_t)region->start;
}

struct Arena* arena_create(size_t size, bool growable) {
    struct ArenaRegion* region = arena_region_create(size);
    if (!region) {
        fprintf(stderr, "%s: failed to create region\n", __func__);
        return NULL;
    }
    struct Arena* arena = (struct Arena*)malloc(sizeof(struct Arena));
    arena->growable = growable;
    arena->regions = region;
    arena->current_region = region;
    return arena;
}

void* arena_alloc(struct Arena* arena, size_t size) {
    uint64_t possible_end = (uintptr_t)arena->current_region->current + size;
    if (possible_end > (uintptr_t)arena->current_region->end) {
        if (!arena->growable) {
            fprintf(stderr, "%s: allocation failed, not growable\n", __func__);
            return NULL;
        }
        uint64_t new_size = arena_region_size(arena->current_region);
        if (size > new_size) {
            new_size = size;
        }
        struct ArenaRegion* new_region = arena_region_create(new_size);
        if (!new_region) {
            fprintf(stderr, "%s: failed to create new region\n", __func__);
            return NULL;
        }
        arena->current_region->next = new_region;
        arena->current_region = new_region;
    }
    struct ArenaRegion* region = arena->current_region;
    void* result = region->current;
    region->current = (void*)((uintptr_t)region->current + size);
    return result;
}

void arena_destroy(struct Arena* arena) {
    struct ArenaRegion* region = arena->regions;
    while (region) {
        struct ArenaRegion* next = region->next;
        arena_region_destroy(region);
        region = next;
    }
    free(arena);
}

static void cputrace_anchor_thread_flush(struct cputrace_anchor* anchor) {
    pthread_mutex_lock(&g_profiler.file_mutex);
    if (anchor->call_count == 0) {
        pthread_mutex_unlock(&g_profiler.file_mutex);
        return;
    }

    const uint64_t overflow_threshold = UINT64_MAX / 2;
    if (anchor->sum_swi > overflow_threshold ||
        anchor->sum_cyc > overflow_threshold ||
        anchor->sum_cmiss > overflow_threshold ||
        anchor->sum_bmiss > overflow_threshold ||
        anchor->sum_ins > overflow_threshold) {
        fprintf(stderr, "Warning: Potential overflow in metrics for '%s'\n",
                anchor->name ? anchor->name : "(null)");
    }

    printf("\nPerformance counter stats for '%s' ('%" PRIu64 " calls):\n\n",
           anchor->name ? anchor->name : "(null)", anchor->call_count);

    char buffer[32];
    auto format_uint64_with_commas = [](uint64_t value, char* buf, size_t buf_size) {
        char temp[32];
        snprintf(temp, sizeof(temp), "%" PRIu64, value);
        int len = strlen(temp);
        int commas = len > 3 ? (len - 1) / 3 : 0;
        int out_len = len + commas;
        if (out_len >= (int)buf_size) return;
        buf[out_len] = '\0';
        int j = out_len - 1;
        int k = 0;
        for (int i = len - 1; i >= 0; i--) {
            buf[j--] = temp[i];
            k++;
            if (k % 3 == 0 && i > 0) {
                buf[j--] = ',';
            }
        }
        while (j >= 0) buf[j--] = ' ';
    };

    auto format_double_with_commas = [](double value, char* buf, size_t buf_size) {
        char temp[32];
        snprintf(temp, sizeof(temp), "%.1f", value);
        int len = strlen(temp);
        int dot_pos = len - 2;
        int whole_len = dot_pos;
        int commas = whole_len > 3 ? (whole_len - 1) / 3 : 0;
        int out_len = len + commas;
        if (out_len >= (int)buf_size) return;
        buf[out_len] = '\0';
        int j = out_len - 1;
        int k = 0;
        for (int i = len - 1; i >= dot_pos; i--) {
            buf[j--] = temp[i];
        }
        for (int i = dot_pos - 1; i >= 0; i--) {
            buf[j--] = temp[i];
            k++;
            if (k % 3 == 0 && i > 0) {
                buf[j--] = ',';
            }
        }
        while (j >= 0) buf[j--] = ' ';
    };

    if (anchor->sum_swi > 0) {
        format_uint64_with_commas(anchor->sum_swi, buffer, sizeof(buffer));
        printf(" %15s %s\n", buffer, "context-switches");
        double avg_swi = static_cast<double>(anchor->sum_swi) / anchor->call_count;
        format_double_with_commas(avg_swi, buffer, sizeof(buffer));
        printf(" %15s %s\n", buffer, "avg context-switches");
    }
    if (anchor->sum_cyc > 0) {
        format_uint64_with_commas(anchor->sum_cyc, buffer, sizeof(buffer));
        printf(" %15s %s\n", buffer, "cycles");
        double avg_cyc = static_cast<double>(anchor->sum_cyc) / anchor->call_count;
        format_double_with_commas(avg_cyc, buffer, sizeof(buffer));
        printf(" %15s %s\n", buffer, "avg cycles");
    }
    if (anchor->sum_cmiss > 0) {
        format_uint64_with_commas(anchor->sum_cmiss, buffer, sizeof(buffer));
        printf(" %15s %s\n", buffer, "cache-misses");
        double avg_cmiss = static_cast<double>(anchor->sum_cmiss) / anchor->call_count;
        format_double_with_commas(avg_cmiss, buffer, sizeof(buffer));
        printf(" %15s %s\n", buffer, "avg cache-misses");
    }
    if (anchor->sum_bmiss > 0) {
        format_uint64_with_commas(anchor->sum_bmiss, buffer, sizeof(buffer));
        printf(" %15s %s\n", buffer, "branch-misses");
        double avg_bmiss = static_cast<double>(anchor->sum_bmiss) / anchor->call_count;
        format_double_with_commas(avg_bmiss, buffer, sizeof(buffer));
        printf(" %15s %s\n", buffer, "avg branch-misses");
    }
    if (anchor->sum_ins > 0) {
        format_uint64_with_commas(anchor->sum_ins, buffer, sizeof(buffer));
        printf(" %15s %s\n", buffer, "instructions");
        double avg_ins = static_cast<double>(anchor->sum_ins) / anchor->call_count;
        format_double_with_commas(avg_ins, buffer, sizeof(buffer));
        printf(" %15s %s\n", buffer, "avg instructions");
    }
    printf("\n");
    fflush(stdout);
    pthread_mutex_unlock(&g_profiler.file_mutex);
}

static void cputrace_result_add(struct cputrace_anchor* anchor,
                              enum cputrace_result_type type, uint64_t value) {
    pthread_mutex_lock(&anchor->mutex);
    switch (type) {
        case CPUTRACE_RESULT_SWI: anchor->sum_swi += value; break;
        case CPUTRACE_RESULT_CYC: anchor->sum_cyc += value; break;
        case CPUTRACE_RESULT_CMISS: anchor->sum_cmiss += value; break;
        case CPUTRACE_RESULT_BMISS: anchor->sum_bmiss += value; break;
        case CPUTRACE_RESULT_INS: anchor->sum_ins += value; break;
        default: break;
    }
    pthread_mutex_unlock(&anchor->mutex);
}

HW_profile::HW_profile(const char* function, uint64_t index, uint64_t flags) {
    if (!g_profiler.profiling) {
        return;
    }
    this->function = function;
    this->index = index;
    this->flags = flags;

    struct HW_conf conf = {0};
    if (flags & HW_PROFILE_SWI) conf.capture_swi = true;
    if (flags & HW_PROFILE_CYC) conf.capture_cyc = true;
    if (flags & HW_PROFILE_CMISS) conf.capture_cmiss = true;
    if (flags & HW_PROFILE_BMISS) conf.capture_bmiss = true;
    if (flags & HW_PROFILE_INS) conf.capture_ins = true;

    HW_init(&ctx, &conf);
    g_profiler.anchors[index].name = function;
    HW_start(&ctx);
}

HW_profile::~HW_profile() {
    if (!g_profiler.profiling) {
        return;
    }
    struct HW_measure measure;
    HW_stop(&ctx, &measure);

    if (flags & HW_PROFILE_SWI) {
        cputrace_result_add(&g_profiler.anchors[index], CPUTRACE_RESULT_SWI, measure.swi);
    }
    if (flags & HW_PROFILE_CYC) {
        cputrace_result_add(&g_profiler.anchors[index], CPUTRACE_RESULT_CYC, measure.cyc);
    }
    if (flags & HW_PROFILE_CMISS) {
        cputrace_result_add(&g_profiler.anchors[index], CPUTRACE_RESULT_CMISS, measure.cmiss);
    }
    if (flags & HW_PROFILE_BMISS) {
        cputrace_result_add(&g_profiler.anchors[index], CPUTRACE_RESULT_BMISS, measure.bmiss);
    }
    if (flags & HW_PROFILE_INS) {
        cputrace_result_add(&g_profiler.anchors[index], CPUTRACE_RESULT_INS, measure.ins);
    }

    pthread_mutex_lock(&g_profiler.anchors[index].mutex);
    g_profiler.anchors[index].call_count++;
    pthread_mutex_unlock(&g_profiler.anchors[index].mutex);

    HW_clean(&ctx);
}

void cputrace_close() {
    if (!g_profiler.profiling) {
        return;
    }
    g_profiler.profiling = false;
    for (uint64_t i = 0; i < CPUTRACE_MAX_ANCHORS; i++) {
        pthread_mutex_lock(&g_profiler.anchors[i].mutex);
        cputrace_anchor_thread_flush(&g_profiler.anchors[i]);
        pthread_mutex_unlock(&g_profiler.anchors[i].mutex);
        pthread_mutex_destroy(&g_profiler.anchors[i].mutex);
        if (g_profiler.anchors[i].results_arena) {
            arena_destroy(g_profiler.anchors[i].results_arena);
            g_profiler.anchors[i].results_arena = NULL;
        }
    }
    pthread_mutex_destroy(&g_profiler.file_mutex);
    if (g_profiler.anchors) {
        // Free the profiler arena (contains anchors)
        struct Arena* profiler_arena = arena_create(sizeof(struct cputrace_anchor) * CPUTRACE_MAX_ANCHORS, true);
        if (profiler_arena) {
            // Reconstruct arena to free anchors
            profiler_arena->regions->current = g_profiler.anchors;
            arena_destroy(profiler_arena);
        }
        g_profiler.anchors = NULL;
    }
}