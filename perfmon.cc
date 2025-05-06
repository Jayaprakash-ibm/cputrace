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

void HW_init(HW_ctx *ctx, HW_conf *conf) {
    ctx->fd_swi = -1;
    ctx->fd_cyc = -1;
    ctx->fd_cmiss = -1;
    ctx->fd_bmiss = -1;
    ctx->fd_ins = -1;
    ctx->conf = *conf;
}

void HW_start(HW_ctx *ctx) {
    struct perf_event_attr pe;

    if (ctx->conf.capture_swi) {
        if (ctx->fd_swi == -1) {
            memset(&pe, 0, sizeof(pe));
            pe.size = sizeof(pe);
            pe.type = PERF_TYPE_SOFTWARE;
            pe.config = PERF_COUNT_SW_CONTEXT_SWITCHES;
            pe.disabled = 1;
            pe.exclude_hv = 1;
            pe.exclude_kernel = 0;
            pe.inherit = 1;
            ctx->fd_swi = perf_event_open(&pe, 0, -1, -1, 0);
            if (ctx->fd_swi == -1) {
                perror("perf_event_open failed for swi");
                ctx->conf.capture_swi = false;
            } else {
                if (ioctl(ctx->fd_swi, PERF_EVENT_IOC_ENABLE, 0) == -1) {
                    perror("ioctl ENABLE failed for swi");
                }
                if (ioctl(ctx->fd_swi, PERF_EVENT_IOC_RESET, 0) == -1) {
                    perror("ioctl RESET failed for swi");
                }
            }
        } else {
            if (ioctl(ctx->fd_swi, PERF_EVENT_IOC_RESET, 0) == -1) {
                perror("ioctl RESET failed for swi");
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
            pe.exclude_hv = 1;
            pe.exclude_kernel = 0; // Include kernel events
            pe.inherit = 1;
            ctx->fd_cyc = perf_event_open(&pe, 0, -1, -1, 0);
            if (ctx->fd_cyc == -1) {
                perror("perf_event_open failed for cyc");
                ctx->conf.capture_cyc = false;
            } else {
                if (ioctl(ctx->fd_cyc, PERF_EVENT_IOC_ENABLE, 0) == -1) {
                    perror("ioctl ENABLE failed for cyc");
                }
                if (ioctl(ctx->fd_cyc, PERF_EVENT_IOC_RESET, 0) == -1) {
                    perror("ioctl RESET failed for cyc");
                }
            }
        } else {
            if (ioctl(ctx->fd_cyc, PERF_EVENT_IOC_RESET, 0) == -1) {
                perror("ioctl RESET failed for cyc");
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
            pe.exclude_hv = 1;
            pe.exclude_kernel = 0; // Include kernel events
            pe.inherit = 1;
            ctx->fd_cmiss = perf_event_open(&pe, 0, -1, -1, 0);
            if (ctx->fd_cmiss == -1) {
                perror("perf_event_open failed for cmiss");
                ctx->conf.capture_cmiss = false;
            } else {
                if (ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_ENABLE, 0) == -1) {
                    perror("ioctl ENABLE failed for cmiss");
                }
                if (ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_RESET, 0) == -1) {
                    perror("ioctl RESET failed for cmiss");
                }
            }
        } else {
            if (ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_RESET, 0) == -1) {
                perror("ioctl RESET failed for cmiss");
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
            pe.exclude_hv = 1;
            pe.exclude_kernel = 0; // Include kernel events
            pe.inherit = 1;
            ctx->fd_bmiss = perf_event_open(&pe, 0, -1, -1, 0);
            if (ctx->fd_bmiss == -1) {
                perror("perf_event_open failed for bmiss");
                ctx->conf.capture_bmiss = false;
            } else {
                if (ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_ENABLE, 0) == -1) {
                    perror("ioctl ENABLE failed for bmiss");
                }
                if (ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_RESET, 0) == -1) {
                    perror("ioctl RESET failed for bmiss");
                }
            }
        } else {
            if (ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_RESET, 0) == -1) {
                perror("ioctl RESET failed for bmiss");
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
            pe.exclude_hv = 1;
            pe.exclude_kernel = 0; // Include kernel events
            pe.inherit = 1;
            ctx->fd_ins = perf_event_open(&pe, 0, -1, -1, 0);
            if (ctx->fd_ins == -1) {
                perror("perf_event_open failed for ins");
                ctx->conf.capture_ins = false;
            } else {
                if (ioctl(ctx->fd_ins, PERF_EVENT_IOC_ENABLE, 0) == -1) {
                    perror("ioctl ENABLE failed for ins");
                }
                if (ioctl(ctx->fd_ins, PERF_EVENT_IOC_RESET, 0) == -1) {
                    perror("ioctl RESET failed for ins");
                }
            }
        } else {
            if (ioctl(ctx->fd_ins, PERF_EVENT_IOC_RESET, 0) == -1) {
                perror("ioctl RESET failed for ins");
            }
        }
    }
}

void HW_stop(HW_ctx *ctx, HW_measure *measure) {
    if (ctx->conf.capture_swi && ctx->fd_swi != -1) {
        if (ioctl(ctx->fd_swi, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            perror("ioctl DISABLE failed for swi");
        }
        if (read(ctx->fd_swi, &measure->swi, sizeof(long long)) == -1) {
            perror("read final failed for swi");
            measure->swi = 0;
        }
    }
    if (ctx->conf.capture_cyc && ctx->fd_cyc != -1) {
        if (ioctl(ctx->fd_cyc, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            perror("ioctl DISABLE failed for cyc");
        }
        if (read(ctx->fd_cyc, &measure->cyc, sizeof(long long)) == -1) {
            perror("read final failed for cyc");
            measure->cyc = 0;
        }
    }
    if (ctx->conf.capture_cmiss && ctx->fd_cmiss != -1) {
        if (ioctl(ctx->fd_cmiss, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            perror("ioctl DISABLE failed for cmiss");
        }
        if (read(ctx->fd_cmiss, &measure->cmiss, sizeof(long long)) == -1) {
            perror("read final failed for cmiss");
            measure->cmiss = 0;
        }
    }
    if (ctx->conf.capture_bmiss && ctx->fd_bmiss != -1) {
        if (ioctl(ctx->fd_bmiss, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            perror("ioctl DISABLE failed for bmiss");
        }
        if (read(ctx->fd_bmiss, &measure->bmiss, sizeof(long long)) == -1) {
            perror("read final failed for bmiss");
            measure->bmiss = 0;
        }
    }
    if (ctx->conf.capture_ins && ctx->fd_ins != -1) {
        if (ioctl(ctx->fd_ins, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            perror("ioctl DISABLE failed for ins");
        }
        if (read(ctx->fd_ins, &measure->ins, sizeof(long long)) == -1) {
            perror("read final failed for ins");
            measure->ins = 0;
        }
    }
}

void HW_clean(HW_ctx *ctx) {
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