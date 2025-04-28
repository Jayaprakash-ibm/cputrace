#ifndef PERFMON_H
#define PERFMON_H

#include <stdint.h>

typedef struct {
    bool capture_swi;
    bool capture_cyc;
    bool capture_cmiss;
    bool capture_bmiss;
    bool capture_ins;
} HW_conf;

typedef struct {
    long long swi;
    long long cyc;
    long long cmiss;
    long long bmiss;
    long long ins;
} HW_metrics;

typedef struct {
    int fd_swi;
    int fd_cyc;
    int fd_cmiss;
    int fd_bmiss;
    int fd_ins;
    HW_conf conf;
    bool initialized;
} HW_ctx;

void HW_init(HW_ctx *ctx, HW_conf conf);
void HW_start(HW_ctx *ctx);
void HW_stop(HW_ctx *ctx, HW_metrics *metrics);
void HW_clean(HW_ctx *ctx);

#endif // PERFMON_H
