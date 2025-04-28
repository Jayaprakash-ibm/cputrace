#include <stdio.h>
#include "perfmon.h"

void my_func_2() {
  for (int i = 0; i < 1000000; i++) {
    int *a = new int;
    delete a;
  }
}

int main() {
    HW_counters counters;
    HW_metrics metrics;

    if (HW_init(&counters) != 0) {
        fprintf(stderr, "Failed to initialize performance monitor.\n");
        return -1;
    }

    if (HW_start(&counters) != 0) {
        fprintf(stderr, "Failed to start performance monitoring.\n");
        HW_cleanup(&counters);
        return -1;
    }

    my_func_2();

    if (HW_stop(&counters, &metrics) != 0) {
        fprintf(stderr, "Failed to stop performance monitoring.\n");
        HW_cleanup(&counters);
        return -1;
    }

    printf("Cache Misses: %lld\n", metrics.cmiss);
    printf("Branch Mispredictions: %lld\n", metrics.bmiss);
    printf("Total Instructions: %lld\n", metrics.ins);

    HW_cleanup(&counters);
    return 0;
}

