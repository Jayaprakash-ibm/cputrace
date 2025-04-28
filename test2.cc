#include "perfmon.h"
#include <stdio.h>

void my_func_2() {
  for (int i = 0; i < 1000000; i++) {
    int *a = new int;
    delete a;
  }
}

int main() {
    HW_ctx ctx = {0};
    HW_conf conf = {false, false, true, true, true}; // Enable cmiss, bmiss, ins
    HW_metrics measure = {0};
    uint64_t my_func_2_cache_misses = 0;
    uint64_t my_func_2_branch_misses = 0;
    uint64_t my_func_2_instructions = 0;

    HW_init(&ctx, conf);

    for (int i = 0; i < 100; i++) {
        HW_start(&ctx);
        my_func_2();
        HW_stop(&ctx, &measure);

        my_func_2_cache_misses += measure.cmiss;
        my_func_2_branch_misses += measure.bmiss;
        my_func_2_instructions += measure.ins;
    }

    printf("my_func_2 Cache Misses: %lu\n", my_func_2_cache_misses);
    printf("my_func_2 Branch Misses: %lu\n", my_func_2_branch_misses);
    printf("my_func_2 Instructions: %lu\n", my_func_2_instructions);

    HW_clean(&ctx);
    return 0;
}
