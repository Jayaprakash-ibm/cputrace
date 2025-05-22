#include "perfmon.h"
#include <stdio.h>
#include <unistd.h>
#include <iostream>

void my_func_1() {
  HWProfileFunctionF(profile, __FUNCTION__, HW_PROFILE_CYC | HW_PROFILE_CMISS | HW_PROFILE_INS | HW_PROFILE_BMISS | HW_PROFILE_SWI);
  for (int i = 0; i < 10000000; i++) {
    int *a = new int;
    delete a;
  }
}

int main() {
    HW_profiler_start profiler;
    for(int i = 0; i < 10; i++)
    	my_func_1();
    std::cout << "Profiling complete. Check profile.log." << std::endl;
    return 0;
}
