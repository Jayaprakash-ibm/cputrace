#ifndef PERFMON_H
#define PERFMON_H

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <linux/perf_event.h>

// Maximum number of threads and anchors (functions) to profile
#define PERFMON_MAX_THREADS 64
#define PERFMON_MAX_ANCHORS 128

struct HW_conf {
    bool capture_swi;
    bool capture_cyc;
    bool capture_cmiss;
    bool capture_bmiss;
    bool capture_ins;
};

struct HW_ctx {
    int fd_swi;
    int fd_cyc;
    int fd_cmiss;
    int fd_bmiss;
    int fd_ins;
    struct HW_conf conf;
};

struct HW_measure {
    long long swi;
    long long cyc;
    long long cmiss;
    long long bmiss;
    long long ins;
};

struct ArenaRegion {
    void* start;
    void* end;
    void* current;
    struct ArenaRegion* next;
};

struct Arena {
    struct ArenaRegion* regions;
    struct ArenaRegion* current_region;
    bool growable;
};

struct perfmon_anchor {
    const char* name;
    pthread_mutex_t mutex[PERFMON_MAX_THREADS];
    struct Arena* results_arena[PERFMON_MAX_THREADS];
    // Added for averaging
    uint64_t call_count[PERFMON_MAX_THREADS];
    uint64_t sum_swi[PERFMON_MAX_THREADS];
    uint64_t sum_cyc[PERFMON_MAX_THREADS];
    uint64_t sum_cmiss[PERFMON_MAX_THREADS];
    uint64_t sum_bmiss[PERFMON_MAX_THREADS];
    uint64_t sum_ins[PERFMON_MAX_THREADS];
};

enum perfmon_result_type {
    PERFMON_RESULT_SWI = 0,
    PERFMON_RESULT_CYC = 1,
    PERFMON_RESULT_CMISS = 2,
    PERFMON_RESULT_BMISS = 3,
    PERFMON_RESULT_INS = 4,
    PERFMON_RESULT_LAST = 5
};

struct perfmon_result {
    enum perfmon_result_type type;
    uint64_t value;
};

struct perfmon_profiler {
    struct perfmon_anchor* anchors;
    bool profiling;
    pthread_mutex_t file_mutex;
};

void HW_init(struct HW_ctx* ctx, struct HW_conf* conf);
void HW_start(struct HW_ctx* ctx);
void HW_stop(struct HW_ctx* ctx, struct HW_measure* measure);
void HW_clean(struct HW_ctx* ctx);

struct Arena* arena_create(size_t size, bool growable);
void* arena_alloc(struct Arena* arena, size_t size);
void arena_destroy(struct Arena* arena);

void perfmon_init(void);
void perfmon_close(void);

// RAII class for profiling a function or scope
struct HW_profile {
    struct HW_ctx ctx;
    const char* function;
    uint64_t index;
    uint64_t flags;
    uint64_t thread_id;

    HW_profile(const char* function, uint64_t index, uint64_t flags);
    ~HW_profile();
};

// Flags for selecting metrics
enum HW_profile_flags {
    HW_PROFILE_SWI = 1,
    HW_PROFILE_CYC = 2,
    HW_PROFILE_CMISS = 4,
    HW_PROFILE_BMISS = 8,
    HW_PROFILE_INS = 16
};

// Macros for easy profiling
#define NameConcat2(A, B) A##B
#define NameConcat(A, B) NameConcat2(A, B)
#define HWProfileFunction(variable, label) \
    struct HW_profile variable(label, (uint64_t)(__COUNTER__ + 1), HW_PROFILE_CYC)
#define HWProfileFunctionF(variable, label, flags) \
    struct HW_profile variable(label, (uint64_t)(__COUNTER__ + 1), flags)

// RAII class for starting the profiler
struct HW_profiler_start {
    struct Arena* profiler_arena;
    HW_profiler_start();
    ~HW_profiler_start();
};

#endif // PERFMON_H
