#ifndef CPUTRACE_H
#define CPUTRACE_H

#include <string>

namespace ceph {
  class Formatter;
}

#define CPUTRACE_MAX_ANCHORS 100

enum cputrace_result_type {
    CPUTRACE_RESULT_SWI = 0,
    CPUTRACE_RESULT_CYC,
    CPUTRACE_RESULT_CMISS,
    CPUTRACE_RESULT_BMISS,
    CPUTRACE_RESULT_INS,
    CPUTRACE_RESULT_COUNT
};

struct ArenaRegion {
    void* start;
    void* current;
    void* end;
    struct ArenaRegion* next;
};

struct Arena {
    bool growable;
    struct ArenaRegion* regions;
    struct ArenaRegion* current_region;
};

struct cputrace_anchor {
    const char* name;
    uint64_t call_count;
    uint64_t sum_swi;
    uint64_t sum_cyc;
    uint64_t sum_cmiss;
    uint64_t sum_bmiss;
    uint64_t sum_ins;
    struct Arena* results_arena;
    pthread_mutex_t mutex;
};

struct cputrace_profiler {
    bool profiling;
    struct cputrace_anchor* anchors;
    pthread_mutex_t file_mutex;
};

struct HW_conf {
    bool capture_swi;
    bool capture_cyc;
    bool capture_cmiss;
    bool capture_bmiss;
    bool capture_ins;
};

struct HW_measure {
    uint64_t swi;
    uint64_t cyc;
    uint64_t cmiss;
    uint64_t bmiss;
    uint64_t ins;
};

struct HW_ctx {
    int fd_swi;
    int fd_cyc;
    int fd_cmiss;
    int fd_bmiss;
    int fd_ins;
    struct HW_conf conf;
};

#define HW_PROFILE_SWI   (1ULL << CPUTRACE_RESULT_SWI)
#define HW_PROFILE_CYC   (1ULL << CPUTRACE_RESULT_CYC)
#define HW_PROFILE_CMISS (1ULL << CPUTRACE_RESULT_CMISS)
#define HW_PROFILE_BMISS (1ULL << CPUTRACE_RESULT_BMISS)
#define HW_PROFILE_INS   (1ULL << CPUTRACE_RESULT_INS)

struct HW_profile {
    const char* function;
    uint64_t index;
    uint64_t flags;
    struct HW_ctx ctx;
    HW_profile(const char* function, uint64_t index, uint64_t flags);
    ~HW_profile();
};

#define HWProfileFunctionF(varname, function, flags) \
    HW_profile varname(function, (uint64_t)(__COUNTER__ + 1), flags)

struct Arena* arena_create(size_t size, bool growable);
void* arena_alloc(struct Arena* arena, size_t size);
void arena_destroy(struct Arena* arena);

void HW_init(struct HW_ctx* ctx, struct HW_conf* conf);
void HW_start(struct HW_ctx* ctx);
void HW_stop(struct HW_ctx* ctx, struct HW_measure* measure);
void HW_clean(struct HW_ctx* ctx);

void cputrace_start(ceph::Formatter* f);
void cputrace_stop(ceph::Formatter* f);
void cputrace_reset(ceph::Formatter* f);
void cputrace_dump(ceph::Formatter* f, const std::string& logger = "", const std::string& counter = "");
void cputrace_close(ceph::Formatter* f);

#endif
