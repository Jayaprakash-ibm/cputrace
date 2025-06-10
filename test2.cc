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
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    double local_sum = 0.0;
    std::vector<int*> allocations;

    for (int i = 0; i < iterations; ++i) {
        double x = dis(gen);
        local_sum += std::sin(x) * std::cos(x) + std::exp(std::sqrt(x));
        if (i % 100 == 0) {
            int* ptr = new int[1024]; // Allocate 4KB
            allocations.push_back(ptr);
            for (int j = 0; j < 1024; ++j) {
                ptr[j] = static_cast<int>(dis(gen) * 1000); // Random writes
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
    HW_profiler_start profiler;

    HW_profile profile(__FUNCTION__, (uint64_t)(__COUNTER__ + 1), HW_PROFILE_CYC | HW_PROFILE_CMISS | HW_PROFILE_INS);

    const int num_threads = std::thread::hardware_concurrency();
    std::cout << "Using " << num_threads << " threads" << std::endl;

    const int iterations = 1000000; // Increased to 1M iterations

    std::vector<std::thread> threads;
    std::vector<double> results(num_threads, 0.0);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_workload, i, iterations, &results[i]);
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Profiling complete. Check profile.log." << std::endl;
    std::cout << "Final Shared Counter: " << g_shared_counter << std::endl;

    return 0;
}