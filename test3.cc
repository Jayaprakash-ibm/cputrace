#include "cputrace.h"
#include <iostream>

void my_function() {
    HWProfileFunctionF(profile, __FUNCTION__, HW_PROFILE_CYC | HW_PROFILE_CMISS | HW_PROFILE_INS);
    for (int i = 0; i < 10000000; i++) { // 10M iterations
        int* a = new int;
        delete a;
    }
}

int main() {
    cputrace_start();
    my_function();
    cputrace_stop();
    cputrace_dump();
    std::cout << "Profiling complete. Check profile.log." << std::endl;
    cputrace_close();
    return 0;
}
