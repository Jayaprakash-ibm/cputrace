#include "perfmon.h"
#include <stdio.h>
#include <unistd.h>

void my_func_1() {
  for (int i = 0; i < 1000000; i++) {
    int *a = new int;
    delete a;
  }
}

int main() {
    HW_ctx ctx = {0};
    HW_conf conf = {true, true, false, false, false}; // Enable swi and cyc
    HW_metrics measure = {0};
    uint64_t my_func_1_no_switch_cnt = 0;
    uint64_t my_func_1_no_switch_cyc = 0;
    uint64_t my_func_1_with_switch_cnt = 0;
    uint64_t my_func_1_with_switch_cyc = 0;

    HW_init(&ctx, conf);

    for (int i = 0; i < 100; i++) {
        HW_start(&ctx);
        my_func_1();
        HW_stop(&ctx, &measure);

        if (measure.swi == 0) {
            my_func_1_no_switch_cnt++;
            my_func_1_no_switch_cyc += measure.cyc;
        } else {
            my_func_1_with_switch_cnt++;
            my_func_1_with_switch_cyc += measure.cyc;
        }
    }

    printf("my_func_1 No Switch: %lu runs, %lu cycles (avg: %.2f)\n",
           my_func_1_no_switch_cnt, my_func_1_no_switch_cyc,
           my_func_1_no_switch_cnt ? (double)my_func_1_no_switch_cyc / my_func_1_no_switch_cnt : 0);
    printf("my_func_1 With Switch: %lu runs, %lu cycles (avg: %.2f)\n",
           my_func_1_with_switch_cnt, my_func_1_with_switch_cyc,
           my_func_1_with_switch_cnt ? (double)my_func_1_with_switch_cyc / my_func_1_with_switch_cnt : 0);

    HW_clean(&ctx);
    return 0;
}
