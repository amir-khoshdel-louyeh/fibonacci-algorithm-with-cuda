#pragma once

#include <vector>

namespace fib {

inline constexpr int kDefaultCount = 5;
inline constexpr int kMaxCount = 47;

struct BenchmarkResult {
    int count = 0;
    double cpu_seconds = 0.0;
    double gpu_seconds = 0.0;
    bool results_match = false;
};

std::vector<int> compute_cpu(int count);
std::vector<int> compute_gpu(int count);
BenchmarkResult run_benchmark(int count);

}  // namespace fib
