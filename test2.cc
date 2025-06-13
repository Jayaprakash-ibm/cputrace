#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <random>
#include <cmath>
#include <chrono>
#include "cputrace.h"

std::mutex g_mutex;
int g_shared_counter = 0;

void thread_workload(int thread_id, int iterations, double* result) {
    HWProfileFunctionF(profile, __FUNCTION__, HW_PROFILE_CYC | HW_PROFILE_CMISS | HW_PROFILE_INS | HW_PROFILE_SWI);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    double local_sum = 0.0;
    std::vector<int*> allocations;

    for (int i = 0; i < iterations; ++i) {
        double x = dis(gen);
        local_sum += std::sin(x) * std::cos(x) + std::exp(std::sqrt(x));
        if (i % 100 == 0) {
            int* ptr = new int[1024];
            allocations.push_back(ptr);
            for (int j = 0; j < 1024; ++j) {
                ptr[j] = static_cast<int>(dis(gen) * 1000);
            }
        }
        if (dis(gen) > 0.5) {
            local_sum += std::tan(x);
        } else {
            local_sum += std::log1p(x);
        }
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_shared_counter++;
            if (g_shared_counter % 1000 == 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }

    for (int* ptr : allocations) {
        delete[] ptr;
    }

    *result = local_sum;

    std::cout << "Thread " << thread_id << " completed with result: " << local_sum << std::endl;
}

int main() {
    freopen("profile.log", "w", stdout);

    std::cout << "Starting test2.cc\n";

    const int num_threads = std::thread::hardware_concurrency();
    std::cout << "Using " << num_threads << " threads\n";

    std::vector<std::thread> threads;
    std::vector<double> results(num_threads, 0.0);

    std::cout << "\n=== Starting profiling ===\n";
    cputrace_start();

    int iterations = 1000000;
    std::cout << "Running first workload with " << iterations << " iterations\n";
    threads.clear();
    g_shared_counter = 0;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_workload, i, iterations, &results[i]);
    }
    for (auto& t : threads) {
        t.join();
    }
    std::cout << "First workload complete. Shared counter: " << g_shared_counter << "\n";

    std::cout << "\n=== Dumping initial results ===\n";
    cputrace_dump();

    std::cout << "\n=== Resetting counters ===\n";
    cputrace_reset();

    iterations = 500000;
    std::cout << "Running second workload with " << iterations << " iterations\n";
    threads.clear();
    g_shared_counter = 0;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_workload, i, iterations, &results[i]);
    }
    for (auto& t : threads) {
        t.join();
    }
    std::cout << "Second workload complete. Shared counter: " << g_shared_counter << "\n";

    std::cout << "\n=== Dumping results after reset ===\n";
    cputrace_dump();

    std::cout << "\n=== Stopping profiling ===\n";
    cputrace_stop();

    iterations = 200000;
    std::cout << "Running third workload with " << iterations << " iterations (no profiling)\n";
    threads.clear();
    g_shared_counter = 0;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_workload, i, iterations, &results[i]);
    }
    for (auto& t : threads) {
        t.join();
    }
    std::cout << "Third workload complete. Shared counter: " << g_shared_counter << "\n";

    std::cout << "\n=== Dumping after stop (should show previous data) ===\n";
    cputrace_dump();

    std::cout << "\n=== Restarting profiling ===\n";
    cputrace_start();

    iterations = 300000;
    std::cout << "Running final workload with " << iterations << " iterations\n";
    threads.clear();
    g_shared_counter = 0;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_workload, i, iterations, &results[i]);
    }
    for (auto& t : threads) {
        t.join();
    }
    std::cout << "Final workload complete. Shared counter: " << g_shared_counter << "\n";

    std::cout << "\n=== Final dump ===\n";
    cputrace_dump();

    std::cout << "\n=== Cleaning up ===\n";
    cputrace_close();

    std::cout << "Test2.cc complete. Check profile.log for results.\n";
    return 0;
}