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

    PerfSamples samples("my_func_1", conf);

    HW_init(&ctx, conf);

    for (int i = 0; i < 100; i++) {
        HW_start(&ctx);
        my_func_1();
        HW_stop(&ctx, &measure);
	samples.add_sample(measure);
    }
    
    print_perf_stats(samples);

    HW_clean(&ctx);
    return 0;
}
