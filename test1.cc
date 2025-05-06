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
    HW_conf conf = {true, true, true, true, true}; // Enable swi and cyc
    HW_measure measure = {0};
    HW_init(&ctx, &conf);

    HW_start(&ctx);
    for (int i = 0; i < 100; i++) {
        my_func_1();
    }
    
    HW_stop(&ctx, &measure);
    printf("swi: %lld\n", measure.swi);
    printf("cyc: %lld\n", measure.cyc);
    printf("cmiss: %lld\n", measure.cmiss);
    printf("bmiss: %lld\n", measure.bmiss);
    printf("ins: %lld\n", measure.ins);
    HW_clean(&ctx);
    return 0;
}
