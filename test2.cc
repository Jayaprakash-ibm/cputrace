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

    PerfSamples samples(std::string("my_func_2"), conf);

    HW_init(&ctx, conf);

    for (int i = 0; i < 100; i++) {
        HW_start(&ctx);
        my_func_2();
        HW_stop(&ctx, &measure);
	samples.add_sample(measure);
    }

    print_perf_stats(samples);

    HW_clean(&ctx);
    return 0;
}
