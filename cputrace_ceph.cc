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
#include <string>
#include "cputrace.h"
#include "common/Formatter.h"

static struct cputrace_profiler g_profiler;

static void initialize_profiler() {
    struct Arena* profiler_arena = arena_create(sizeof(struct cputrace_anchor) * CPUTRACE_MAX_ANCHORS, true);
    if (!profiler_arena) {
        exit(1);
    }
    g_profiler.anchors = (struct cputrace_anchor*)arena_alloc(profiler_arena, sizeof(struct cputrace_anchor) * CPUTRACE_MAX_ANCHORS);
    if (!g_profiler.anchors) {
        arena_destroy(profiler_arena);
        exit(1);
    }
    memset(g_profiler.anchors, 0, sizeof(struct cputrace_anchor) * CPUTRACE_MAX_ANCHORS);
    for (uint64_t i = 0; i < CPUTRACE_MAX_ANCHORS; i++) {
        pthread_mutex_init(&g_profiler.anchors[i].mutex, NULL);
        g_profiler.anchors[i].results_arena = arena_create(20 * 1024 * 1024, false);
        if (!g_profiler.anchors[i].results_arena) {
            for (uint64_t j = 0; j < i; j++) {
                arena_destroy(g_profiler.anchors[j].results_arena);
                pthread_mutex_destroy(&g_profiler.anchors[j].mutex);
            }
            arena_destroy(profiler_arena);
            exit(1);
        }
    }
    g_profiler.profiling = false;
    pthread_mutex_init(&g_profiler.file_mutex, NULL);
}

static struct ProfilerInitializer {
    ProfilerInitializer() { initialize_profiler(); }
} profiler_initializer;

static long perf_event_open(struct perf_event_attr* hw_event, pid_t pid,
                           int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
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
                ioctl(ctx->fd_swi, PERF_EVENT_IOC_RESET, 0);
                ioctl(ctx->fd_swi, PERF_EVENT_IOC_ENABLE, 0);
            } else {
                ctx->conf.capture_swi = false;
            }
        } else {
            ioctl(ctx->fd_swi, PERF_EVENT_IOC_RESET, 0);
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
                ioctl(ctx->fd_cyc, PERF_EVENT_IOC_RESET, 0);
                ioctl(ctx->fd_cyc, PERF_EVENT_IOC_ENABLE, 0);
            } else {
                ctx->conf.capture_cyc = false;
            }
        } else {
            ioctl(ctx->fd_cyc, PERF_EVENT_IOC_RESET, 0);
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
                ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_RESET, 0);
                ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_ENABLE, 0);
            } else {
                ctx->conf.capture_cmiss = false;
            }
        } else {
            ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_RESET, 0);
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
                ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_RESET, 0);
                ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_ENABLE, 0);
            } else {
                ctx->conf.capture_bmiss = false;
            }
        } else {
            ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_RESET, 0);
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
                ioctl(ctx->fd_ins, PERF_EVENT_IOC_RESET, 0);
                ioctl(ctx->fd_ins, PERF_EVENT_IOC_ENABLE, 0);
            } else {
                ctx->conf.capture_ins = false;
            }
        } else {
            ioctl(ctx->fd_ins, PERF_EVENT_IOC_RESET, 0);
        }
    }
}

void HW_stop(struct HW_ctx* ctx, struct HW_measure* measure) {
    if (ctx->conf.capture_swi && ctx->fd_swi != -1) {
        ioctl(ctx->fd_swi, PERF_EVENT_IOC_DISABLE, 0);
        long long value = 0;
        if (read(ctx->fd_swi, &value, sizeof(long long)) != -1) {
            measure->swi = value;
        } else {
            measure->swi = 0;
        }
    }
    if (ctx->conf.capture_cyc && ctx->fd_cyc != -1) {
        ioctl(ctx->fd_cyc, PERF_EVENT_IOC_DISABLE, 0);
        long long value = 0;
        if (read(ctx->fd_cyc, &value, sizeof(long long)) != -1) {
            measure->cyc = value;
        } else {
            measure->cyc = 0;
        }
    }
    if (ctx->conf.capture_cmiss && ctx->fd_cmiss != -1) {
        ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_DISABLE, 0);
        long long value = 0;
        if (read(ctx->fd_cmiss, &value, sizeof(long long)) != -1) {
            measure->cmiss = value;
        } else {
            measure->cmiss = 0;
        }
    }
    if (ctx->conf.capture_bmiss && ctx->fd_bmiss != -1) {
        ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_DISABLE, 0);
        long long value = 0;
        if (read(ctx->fd_bmiss, &value, sizeof(long long)) != -1) {
            measure->bmiss = value;
        } else {
            measure->bmiss = 0;
        }
    }
    if (ctx->conf.capture_ins && ctx->fd_ins != -1) {
        ioctl(ctx->fd_ins, PERF_EVENT_IOC_DISABLE, 0);
        long long value = 0;
        if (read(ctx->fd_ins, &value, sizeof(long long)) != -1) {
            measure->ins = value;
        } else {
            measure->ins = 0;
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

static void cputrace_anchor_thread_flush(struct cputrace_anchor* anchor, ceph::Formatter* f, const std::string& counter) {
    if (anchor->call_count == 0) {
        return;
    }

    f->open_object_section(anchor->name ? anchor->name : "unknown");
    f->dump_unsigned("call_count", anchor->call_count);

    if (counter.empty() || counter == "context_switches") {
        if (anchor->sum_swi > 0) {
            f->dump_unsigned("context_switches", anchor->sum_swi);
            f->dump_float("avg_context_switches", static_cast<double>(anchor->sum_swi) / anchor->call_count);
        }
    }
    if (counter.empty() || counter == "cycles") {
        if (anchor->sum_cyc > 0) {
            f->dump_unsigned("cycles", anchor->sum_cyc);
            f->dump_float("avg_cycles", static_cast<double>(anchor->sum_cyc) / anchor->call_count);
        }
    }
    if (counter.empty() || counter == "cache_misses") {
        if (anchor->sum_cmiss > 0) {
            f->dump_unsigned("cache_misses", anchor->sum_cmiss);
            f->dump_float("avg_cache_misses", static_cast<double>(anchor->sum_cmiss) / anchor->call_count);
        }
    }
    if (counter.empty() || counter == "branch_misses") {
        if (anchor->sum_bmiss > 0) {
            f->dump_unsigned("branch_misses", anchor->sum_bmiss);
            f->dump_float("avg_branch_misses", static_cast<double>(anchor->sum_bmiss) / anchor->call_count);
        }
    }
    if (counter.empty() || counter == "instructions") {
        if (anchor->sum_ins > 0) {
            f->dump_unsigned("instructions", anchor->sum_ins);
            f->dump_float("avg_instructions", static_cast<double>(anchor->sum_ins) / anchor->call_count);
        }
    }
    f->close_section();
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
    if (index >= CPUTRACE_MAX_ANCHORS) {
        return;
    }
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

void cputrace_start(ceph::Formatter* f) {
    pthread_mutex_lock(&g_profiler.file_mutex);
    if (g_profiler.profiling) {
        f->open_object_section("cputrace_start");
        f->dump_format("status", "Profiling already active");
        f->close_section();
        pthread_mutex_unlock(&g_profiler.file_mutex);
        return;
    }
    g_profiler.profiling = true;
    f->open_object_section("cputrace_start");
    f->dump_format("status", "Profiling started");
    f->close_section();
    pthread_mutex_unlock(&g_profiler.file_mutex);
}

void cputrace_stop(ceph::Formatter* f) {
    pthread_mutex_lock(&g_profiler.file_mutex);
    if (!g_profiler.profiling) {
        f->open_object_section("cputrace_stop");
        f->dump_format("status", "Profiling not active");
        f->close_section();
        pthread_mutex_unlock(&g_profiler.file_mutex);
        return;
    }
    g_profiler.profiling = false;
    f->open_object_section("cputrace_stop");
    f->dump_format("status", "Profiling stopped");
    f->close_section();
    pthread_mutex_unlock(&g_profiler.file_mutex);
}

void cputrace_reset(ceph::Formatter* f) {
    pthread_mutex_lock(&g_profiler.file_mutex);
    for (uint64_t i = 0; i < CPUTRACE_MAX_ANCHORS; i++) {
        pthread_mutex_lock(&g_profiler.anchors[i].mutex);
        g_profiler.anchors[i].call_count = 0;
        g_profiler.anchors[i].sum_swi = 0;
        g_profiler.anchors[i].sum_cyc = 0;
        g_profiler.anchors[i].sum_cmiss = 0;
        g_profiler.anchors[i].sum_bmiss = 0;
        g_profiler.anchors[i].sum_ins = 0;
        pthread_mutex_unlock(&g_profiler.anchors[i].mutex);
    }
    f->open_object_section("cputrace_reset");
    f->dump_format("status", "Profiling counters reset");
    f->close_section();
    pthread_mutex_unlock(&g_profiler.file_mutex);
}

void cputrace_dump(ceph::Formatter* f, const std::string& logger, const std::string& counter) {
    pthread_mutex_lock(&g_profiler.file_mutex);
    f->open_object_section("cputrace");
    bool dumped = false;

    for (uint64_t i = 0; i < CPUTRACE_MAX_ANCHORS; i++) {
        pthread_mutex_lock(&g_profiler.anchors[i].mutex);
        if (g_profiler.anchors[i].name && g_profiler.anchors[i].call_count > 0) {
            std::string anchor_name = g_profiler.anchors[i].name;
            if (logger.empty() || anchor_name == logger) {
                cputrace_anchor_thread_flush(&g_profiler.anchors[i], f, counter);
                dumped = true;
            }
        }
        pthread_mutex_unlock(&g_profiler.anchors[i].mutex);
    }

    if (!dumped) {
        f->dump_format("status", "No profiling data available");
    } else {
        f->dump_format("status", "Profiling data dumped");
    }
    f->close_section();
    pthread_mutex_unlock(&g_profiler.file_mutex);
}

void cputrace_close(ceph::Formatter* f) {
    pthread_mutex_lock(&g_profiler.file_mutex);
    if (!g_profiler.anchors) {
        f->open_object_section("cputrace_close");
        f->dump_format("status", "Profiler not initialized");
        f->close_section();
        pthread_mutex_unlock(&g_profiler.file_mutex);
        return;
    }
    for (uint64_t i = 0; i < CPUTRACE_MAX_ANCHORS; i++) {
        pthread_mutex_lock(&g_profiler.anchors[i].mutex);
        if (g_profiler.anchors[i].results_arena) {
            arena_destroy(g_profiler.anchors[i].results_arena);
            g_profiler.anchors[i].results_arena = NULL;
        }
        pthread_mutex_unlock(&g_profiler.anchors[i].mutex);
        pthread_mutex_destroy(&g_profiler.anchors[i].mutex);
    }
    if (g_profiler.anchors) {
        struct Arena* profiler_arena = arena_create(sizeof(struct cputrace_anchor) * CPUTRACE_MAX_ANCHORS, true);
        if (profiler_arena) {
            profiler_arena->regions->current = g_profiler.anchors;
            arena_destroy(profiler_arena);
        }
        g_profiler.anchors = NULL;
    }
    f->open_object_section("cputrace_close");
    f->dump_format("status", "Profiling closed");
    f->close_section();
    pthread_mutex_destroy(&g_profiler.file_mutex);
}
