#ifndef CPUTRACE_H
#define CPUTRACE_H

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <linux/perf_event.h>

// Maximum number of threads and anchors (functions) to profile
#define CPUTRACE_MAX_THREADS 64
#define CPUTRACE_MAX_ANCHORS 128

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

struct cputrace_anchor {
    const char* name;
    pthread_mutex_t mutex[CPUTRACE_MAX_THREADS];
    struct Arena* results_arena[CPUTRACE_MAX_THREADS];
    // Added for averaging
    uint64_t call_count[CPUTRACE_MAX_THREADS];
    uint64_t sum_swi[CPUTRACE_MAX_THREADS];
    uint64_t sum_cyc[CPUTRACE_MAX_THREADS];
    uint64_t sum_cmiss[CPUTRACE_MAX_THREADS];
    uint64_t sum_bmiss[CPUTRACE_MAX_THREADS];
    uint64_t sum_ins[CPUTRACE_MAX_THREADS];
};

enum cputrace_result_type {
    CPUTRACE_RESULT_SWI = 0,
    CPUTRACE_RESULT_CYC = 1,
    CPUTRACE_RESULT_CMISS = 2,
    CPUTRACE_RESULT_BMISS = 3,
    CPUTRACE_RESULT_INS = 4,
    CPUTRACE_RESULT_LAST = 5
};

struct cputrace_result {
    enum cputrace_result_type type;
    uint64_t value;
};

struct cputrace_profiler {
    struct cputrace_anchor* anchors;
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

void cputrace_init(void);
void cputrace_close(void);

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

#endif // CPUTRACE_H
