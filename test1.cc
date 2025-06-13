#include "cputrace.h"
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include "cputrace.h"

void test_func_1() {
    HWProfileFunctionF(profile, __FUNCTION__, HW_PROFILE_CYC | HW_PROFILE_INS);
    for (int i = 0; i < 10000000; i++) {
        volatile int x = i * i;
        (void)x;
    }
}

void test_func_2() {
    HWProfileFunctionF(profile, __FUNCTION__, HW_PROFILE_CMISS | HW_PROFILE_BMISS);
    int* array = new int[1000000];
    for (int i = 0; i < 1000000; i += 100) {
        array[i] = i;
    }
    delete[] array;
}

void test_func_3() {
    HWProfileFunctionF(profile, __FUNCTION__, HW_PROFILE_SWI);
    for (int i = 0; i < 100; i++) {
        usleep(1000);
    }
}

int main() {
    std::cout << "Starting test1.cc\n";

    std::cout << "\n=== Starting profiling ===\n";
    cputrace_start();
    for (int i = 0; i < 5; i++) {
        test_func_1();
        test_func_2();
        test_func_3();
    }

    std::cout << "\n=== Dumping initial results ===\n";
    cputrace_dump();

    std::cout << "\n=== Resetting counters ===\n";
    cputrace_reset();
    for (int i = 0; i < 3; i++) {
        test_func_1();
        test_func_2();
    }

    std::cout << "\n=== Dumping results after reset ===\n";
    cputrace_dump();

    std::cout << "\n=== Stopping profiling ===\n";
    cputrace_stop();
    for (int i = 0; i < 2; i++) {
        test_func_1();
        test_func_3();
    }

    std::cout << "\n=== Dumping after stop (should show previous data) ===\n";
    cputrace_dump();

    std::cout << "\n=== Restarting profiling ===\n";
    cputrace_start();
    for (int i = 0; i < 4; i++) {
        test_func_2();
        test_func_3();
    }

    std::cout << "\n=== Final dump ===\n";
    cputrace_dump();

    std::cout << "\n=== Cleaning up ===\n";
    cputrace_close();

    std::cout << "Test1.cc complete.\n";
    return 0;
}