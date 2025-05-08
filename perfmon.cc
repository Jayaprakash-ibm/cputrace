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
#include "perfmon.h"

static struct perfmon_profiler g_profiler;

// Syscall wrapper for perf_event_open
static long perf_event_open(struct perf_event_attr* hw_event, pid_t pid,
                           int cpu, int group_fd, unsigned long flags) {
    long fd = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
    if (fd == -1) {
        fprintf(stderr, "perf_event_open failed: %s (errno=%d)\n", strerror(errno), errno);
    }
    return fd;
}

// Initialize the context with configuration
void HW_init(struct HW_ctx* ctx, struct HW_conf* conf) {
    ctx->fd_swi = -1;
    ctx->fd_cyc = -1;
    ctx->fd_cmiss = -1;
    ctx->fd_bmiss = -1;
    ctx->fd_ins = -1;
    ctx->conf = *conf;
}

// Start profiling for the given context
void HW_start(struct HW_ctx* ctx) {
    struct perf_event_attr pe;

    if (ctx->conf.capture_swi) {
        if (ctx->fd_swi == -1) {
            memset(&pe, 0, sizeof(pe));
            pe.size = sizeof(pe);
            pe.type = PERF_TYPE_SOFTWARE;
            pe.config = PERF_COUNT_SW_CONTEXT_SWITCHES;
            pe.disabled = 1;
            pe.inherit = 1; // Inherit counters to all threads
            ctx->fd_swi = perf_event_open(&pe, 0, -1, -1, 0);
            if (ctx->fd_swi != -1) {
                printf("Opened swi counter, fd=%d\n", ctx->fd_swi);
                if (ioctl(ctx->fd_swi, PERF_EVENT_IOC_RESET, 0) == -1) {
                    fprintf(stderr, "ioctl RESET failed for swi: %s\n", strerror(errno));
                }
                if (ioctl(ctx->fd_swi, PERF_EVENT_IOC_ENABLE, 0) == -1) {
                    fprintf(stderr, "ioctl ENABLE failed for swi: %s\n", strerror(errno));
                }
            } else {
                ctx->conf.capture_swi = false;
                fprintf(stderr, "Failed to open swi counter\n");
            }
        } else {
            if (ioctl(ctx->fd_swi, PERF_EVENT_IOC_RESET, 0) == -1) {
                fprintf(stderr, "ioctl RESET failed for swi: %s\n", strerror(errno));
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
            pe.inherit = 1; // Inherit counters to all threads
            ctx->fd_cyc = perf_event_open(&pe, 0, -1, -1, 0);
            if (ctx->fd_cyc != -1) {
                printf("Opened cyc counter, fd=%d\n", ctx->fd_cyc);
                if (ioctl(ctx->fd_cyc, PERF_EVENT_IOC_RESET, 0) == -1) {
                    fprintf(stderr, "ioctl RESET failed for cyc: %s\n", strerror(errno));
                }
                if (ioctl(ctx->fd_cyc, PERF_EVENT_IOC_ENABLE, 0) == -1) {
                    fprintf(stderr, "ioctl ENABLE failed for cyc: %s\n", strerror(errno));
                }
            } else {
                ctx->conf.capture_cyc = false;
                fprintf(stderr, "Failed to open cyc counter\n");
            }
        } else {
            if (ioctl(ctx->fd_cyc, PERF_EVENT_IOC_RESET, 0) == -1) {
                fprintf(stderr, "ioctl RESET failed for cyc: %s\n", strerror(errno));
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
            pe.inherit = 1; // Inherit counters to all threads
            ctx->fd_cmiss = perf_event_open(&pe, 0, -1, -1, 0);
            if (ctx->fd_cmiss != -1) {
                printf("Opened cmiss counter, fd=%d\n", ctx->fd_cmiss);
                if (ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_RESET, 0) == -1) {
                    fprintf(stderr, "ioctl RESET failed for cmiss: %s\n", strerror(errno));
                }
                if (ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_ENABLE, 0) == -1) {
                    fprintf(stderr, "ioctl ENABLE failed for cmiss: %s\n", strerror(errno));
                }
            } else {
                ctx->conf.capture_cmiss = false;
                fprintf(stderr, "Failed to open cmiss counter\n");
            }
        } else {
            if (ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_RESET, 0) == -1) {
                fprintf(stderr, "ioctl RESET failed for cmiss: %s\n", strerror(errno));
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
            pe.inherit = 1; // Inherit counters to all threads
            ctx->fd_bmiss = perf_event_open(&pe, 0, -1, -1, 0);
            if (ctx->fd_bmiss != -1) {
                printf("Opened bmiss counter, fd=%d\n", ctx->fd_bmiss);
                if (ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_RESET, 0) == -1) {
                    fprintf(stderr, "ioctl RESET failed for bmiss: %s\n", strerror(errno));
                }
                if (ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_ENABLE, 0) == -1) {
                    fprintf(stderr, "ioctl ENABLE failed for bmiss: %s\n", strerror(errno));
                }
            } else {
                ctx->conf.capture_bmiss = false;
                fprintf(stderr, "Failed to open bmiss counter\n");
            }
        } else {
            if (ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_RESET, 0) == -1) {
                fprintf(stderr, "ioctl RESET failed for bmiss: %s\n", strerror(errno));
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
            pe.inherit = 1; // Inherit counters to all threads
            ctx->fd_ins = perf_event_open(&pe, 0, -1, -1, 0);
            if (ctx->fd_ins != -1) {
                printf("Opened ins counter, fd=%d\n", ctx->fd_ins);
                if (ioctl(ctx->fd_ins, PERF_EVENT_IOC_RESET, 0) == -1) {
                    fprintf(stderr, "ioctl RESET failed for ins: %s\n", strerror(errno));
                }
                if (ioctl(ctx->fd_ins, PERF_EVENT_IOC_ENABLE, 0) == -1) {
                    fprintf(stderr, "ioctl ENABLE failed for ins: %s\n", strerror(errno));
                }
            } else {
                ctx->conf.capture_ins = false;
                fprintf(stderr, "Failed to open ins counter\n");
            }
        } else {
            if (ioctl(ctx->fd_ins, PERF_EVENT_IOC_RESET, 0) == -1) {
                fprintf(stderr, "ioctl RESET failed for ins: %s\n", strerror(errno));
            }
        }
    }
}

// Stop profiling and collect measurements
void HW_stop(struct HW_ctx* ctx, struct HW_measure* measure) {
    if (ctx->conf.capture_swi && ctx->fd_swi != -1) {
        if (ioctl(ctx->fd_swi, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            fprintf(stderr, "ioctl DISABLE failed for swi: %s\n", strerror(errno));
        }
        long long value;
        if (read(ctx->fd_swi, &value, sizeof(long long)) == -1) {
            fprintf(stderr, "read final failed for swi: %s\n", strerror(errno));
            measure->swi = 0;
        } else {
            measure->swi = value;
            printf("Read swi: %lld\n", value);
        }
    }
    if (ctx->conf.capture_cyc && ctx->fd_cyc != -1) {
        if (ioctl(ctx->fd_cyc, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            fprintf(stderr, "ioctl DISABLE failed for cyc: %s\n", strerror(errno));
        }
        long long value;
        if (read(ctx->fd_cyc, &value, sizeof(long long)) == -1) {
            fprintf(stderr, "read final failed for cyc: %s\n", strerror(errno));
            measure->cyc = 0;
        } else {
            measure->cyc = value;
            printf("Read cyc: %lld\n", value);
        }
    }
    if (ctx->conf.capture_cmiss && ctx->fd_cmiss != -1) {
        if (ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            fprintf(stderr, "ioctl DISABLE failed for cmiss: %s\n", strerror(errno));
        }
        long long value;
        if (read(ctx->fd_cmiss, &value, sizeof(long long)) == -1) {
            fprintf(stderr, "read final failed for cmiss: %s\n", strerror(errno));
            measure->cmiss = 0;
        } else {
            measure->cmiss = value;
            printf("Read cmiss: %lld\n", value);
        }
    }
    if (ctx->conf.capture_bmiss && ctx->fd_bmiss != -1) {
        if (ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            fprintf(stderr, "ioctl DISABLE failed for bmiss: %s\n", strerror(errno));
        }
        long long value;
        if (read(ctx->fd_bmiss, &value, sizeof(long long)) == -1) {
            fprintf(stderr, "read final failed for bmiss: %s\n", strerror(errno));
            measure->bmiss = 0;
        } else {
            measure->bmiss = value;
            printf("Read bmiss: %lld\n", value);
        }
    }
    if (ctx->conf.capture_ins && ctx->fd_ins != -1) {
        if (ioctl(ctx->fd_ins, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            fprintf(stderr, "ioctl DISABLE failed for ins: %s\n", strerror(errno));
        }
        long long value;
        if (read(ctx->fd_ins, &value, sizeof(long long)) == -1) {
            fprintf(stderr, "read final failed for ins: %s\n", strerror(errno));
            measure->ins = 0;
        } else {
            measure->ins = value;
            printf("Read ins: %lld\n", value);
        }
    }
}

// Clean up context resources
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

// Arena region management
static struct ArenaRegion* arena_region_create(size_t size) {
    void* start = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (start == MAP_FAILED) {
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

// Arena management
struct Arena* arena_create(size_t size, bool growable) {
    struct ArenaRegion* region = arena_region_create(size);
    if (!region) {
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
            return NULL;
        }
        uint64_t new_size = arena_region_size(arena->current_region);
        if (size > new_size) {
            new_size = size;
        }
        struct ArenaRegion* new_region = arena_region_create(new_size);
        if (!new_region) {
            fprintf(stderr, "Error: arena_region_create failed\n");
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

// Flush profiling data for a thread
static void perfmon_anchor_thread_flush(struct perfmon_anchor* anchor, uint64_t thread_id) {
    pthread_mutex_lock(&g_profiler.file_mutex);
    struct Arena* arena = anchor->results_arena[thread_id];
    if (pthread_mutex_trylock(&anchor->mutex[thread_id]) != EBUSY) {
        fprintf(stderr, "Error: mutex not locked\n");
        exit(EXIT_FAILURE);
    }
    FILE* log_file = g_profiler.log_file;
    uint64_t amount = (uintptr_t)arena->current_region->current - (uintptr_t)arena->current_region->start;
    if (amount > 0) {
        fprintf(log_file, "Function: %s, Thread: %lu\n", anchor->name, thread_id);
        struct perfmon_result* result = (struct perfmon_result*)arena->current_region->start;
        uint64_t count = amount / sizeof(struct perfmon_result);
        for (uint64_t i = 0; i < count; i++) {
            printf("Flushing result: thread=%lu, type=%d, value=%lu\n", thread_id, result[i].type, result[i].value);
            const char* metric;
            switch (result[i].type) {
                case PERFMON_RESULT_SWI: metric = "Context Switches"; break;
                case PERFMON_RESULT_CYC: metric = "CPU Cycles"; break;
                case PERFMON_RESULT_CMISS: metric = "Cache Misses"; break;
                case PERFMON_RESULT_BMISS: metric = "Branch Misses"; break;
                case PERFMON_RESULT_INS: metric = "Instructions"; break;
                default: metric = "Unknown"; break;
            }
            fprintf(log_file, "  %s: %lu\n", metric, result[i].value);
        }
        fprintf(log_file, "\n");
        fflush(log_file); // Ensure data is written to disk
    }
    arena->current_region->current = arena->current_region->start;
    pthread_mutex_unlock(&g_profiler.file_mutex);
}

// Add a profiling result
static void perfmon_result_add(struct perfmon_anchor* anchor, uint64_t thread_id,
                              enum perfmon_result_type type, uint64_t value) {
    printf("Adding result: thread=%lu, type=%d, value=%lu\n", thread_id, type, value);
    pthread_mutex_lock(&anchor->mutex[thread_id]);
    struct Arena* arena = anchor->results_arena[thread_id];
    struct perfmon_result* result = (struct perfmon_result*)arena_alloc(arena, sizeof(struct perfmon_result));
    if (!result) {
        perfmon_anchor_thread_flush(anchor, thread_id);
        result = (struct perfmon_result*)arena_alloc(arena, sizeof(struct perfmon_result));
    }
    result->type = type;
    result->value = value;
    pthread_mutex_unlock(&anchor->mutex[thread_id]);
}

// HW_profile constructor
HW_profile::HW_profile(const char* function, uint64_t index, uint64_t flags) {
    if (!g_profiler.profiling) {
        return;
    }
    this->function = function;
    this->index = index;
    this->flags = flags;

    // Initialize thread ID
    this->thread_id = pthread_self() % PERFMON_MAX_THREADS;

    // Set up configuration
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

// HW_profile destructor
HW_profile::~HW_profile() {
    if (!g_profiler.profiling) {
        return;
    }
    struct HW_measure measure;
    HW_stop(&ctx, &measure);

    // Log raw counter values
    if (flags & HW_PROFILE_SWI) {
        perfmon_result_add(&g_profiler.anchors[index], thread_id, PERFMON_RESULT_SWI, measure.swi);
    }
    if (flags & HW_PROFILE_CYC) {
        perfmon_result_add(&g_profiler.anchors[index], thread_id, PERFMON_RESULT_CYC, measure.cyc);
    }
    if (flags & HW_PROFILE_CMISS) {
        perfmon_result_add(&g_profiler.anchors[index], thread_id, PERFMON_RESULT_CMISS, measure.cmiss);
    }
    if (flags & HW_PROFILE_BMISS) {
        perfmon_result_add(&g_profiler.anchors[index], thread_id, PERFMON_RESULT_BMISS, measure.bmiss);
    }
    if (flags & HW_PROFILE_INS) {
        perfmon_result_add(&g_profiler.anchors[index], thread_id, PERFMON_RESULT_INS, measure.ins);
    }

    HW_clean(&ctx);
}

// Initialize log file
void perfmon_init_log_file(const char* filename) {
    g_profiler.profiling = true;
    g_profiler.log_file = fopen(filename, "w"); // Use text mode
    if (!g_profiler.log_file) {
        fprintf(stderr, "Error: fopen failed for %s: %s\n", filename, strerror(errno));
        g_profiler.profiling = false;
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(&g_profiler.file_mutex, NULL);
}

// Close log file and clean up
void perfmon_close_log_file() {
    g_profiler.profiling = false;
    for (uint64_t i = 0; i < PERFMON_MAX_ANCHORS; i++) {
        for (uint64_t j = 0; j < PERFMON_MAX_THREADS; j++) {
            pthread_mutex_lock(&g_profiler.anchors[i].mutex[j]);
            perfmon_anchor_thread_flush(&g_profiler.anchors[i], j);
            pthread_mutex_unlock(&g_profiler.anchors[i].mutex[j]);
            pthread_mutex_destroy(&g_profiler.anchors[i].mutex[j]);
            if (g_profiler.anchors[i].results_arena[j]) {
                arena_destroy(g_profiler.anchors[i].results_arena[j]);
            }
        }
    }
    if (g_profiler.log_file) {
        fclose(g_profiler.log_file);
        g_profiler.log_file = NULL;
    }
    pthread_mutex_destroy(&g_profiler.file_mutex);
}

// HW_profiler_start constructor
HW_profiler_start::HW_profiler_start(const char* filename) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%d-%s", getpid(), filename);
    profiler_arena = arena_create(sizeof(struct perfmon_anchor) * PERFMON_MAX_ANCHORS, true);
    if (!profiler_arena) {
        fprintf(stderr, "Error: arena_create failed\n");
        exit(EXIT_FAILURE);
    }
    g_profiler.anchors = (struct perfmon_anchor*)arena_alloc(profiler_arena, sizeof(struct perfmon_anchor) * PERFMON_MAX_ANCHORS);
    if (!g_profiler.anchors) {
        fprintf(stderr, "Error: arena_alloc anchors failed\n");
        exit(EXIT_FAILURE);
    }
    memset(g_profiler.anchors, 0, sizeof(struct perfmon_anchor) * PERFMON_MAX_ANCHORS);
    for (uint64_t i = 0; i < PERFMON_MAX_ANCHORS; i++) {
        for (uint64_t j = 0; j < PERFMON_MAX_THREADS; j++) {
            pthread_mutex_init(&g_profiler.anchors[i].mutex[j], NULL);
            g_profiler.anchors[i].results_arena[j] = arena_create(20 * 1024 * 1024, false);
        }
    }
    perfmon_init_log_file(buffer);
}

// HW_profiler_start destructor
HW_profiler_start::~HW_profiler_start() {
    if (g_profiler.profiling) {
        perfmon_close_log_file();
        arena_destroy(profiler_arena);
    }
}