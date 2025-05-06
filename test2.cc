#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <random>
#include <cmath>
#include <chrono>
#include "perfmon.h"

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
    HW_ctx ctx = {0};
    HW_conf conf = {true, true, true, true, true}; // Enable all counters
    HW_measure measure = {0};
    HW_init(&ctx, &conf);

    const int num_threads = std::thread::hardware_concurrency();
    std::cout << "Using " << num_threads << " threads" << std::endl;

    const int iterations = 100000;

    std::vector<std::thread> threads;
    std::vector<double> results(num_threads, 0.0);

    HW_start(&ctx);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_workload, i, iterations, &results[i]);
    }

    for (auto& t : threads) {
        t.join();
    }

    HW_stop(&ctx, &measure);

    std::cout << "Performance Counter Results:" << std::endl;
    std::cout << "Context Switches: " << (measure.swi) << std::endl;
    std::cout << "CPU Cycles: " << (measure.cyc) << std::endl;
    std::cout << "Cache Misses: " << (measure.cmiss) << std::endl;
    std::cout << "Branch Misses: " << (measure.bmiss) << std::endl;
    std::cout << "Instructions: " << (measure.ins) << std::endl;

    HW_clean(&ctx);

    std::cout << "Final Shared Counter: " << g_shared_counter << std::endl;

    return 0;
}
