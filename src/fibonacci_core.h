#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace fib {

inline constexpr int kDefaultCount = 5;
inline constexpr int kMaxCount = 47;

struct BenchmarkResult {
    int count = 0;
    int iterations = 0;
    double cpu_seconds = 0.0;
    double gpu_seconds = 0.0;
    double gpu_kernel_seconds = 0.0;
    bool results_match = false;
};

struct BenchmarkOptions {
    int iterations = 3;
    int warmup_iterations = 1;
};

struct DeviceInfo {
    bool available = false;
    std::string name;
    int compute_major = 0;
    int compute_minor = 0;
    int multiprocessor_count = 0;
    std::size_t total_memory_bytes = 0;
    std::string error;
};

DeviceInfo query_device();

std::vector<int> compute_cpu(int count);
std::vector<int> compute_gpu(int count);
BenchmarkResult run_benchmark(int count,
                              const BenchmarkOptions& options = {});

}  // namespace fib
