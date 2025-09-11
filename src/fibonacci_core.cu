#include "fibonacci_core.h"

#include <chrono>
#include <cuda_runtime.h>
#include <stdexcept>
#include <string>

namespace fib {
namespace {

int fibonacci_cpu(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci_cpu(n - 1) + fibonacci_cpu(n - 2);
}

__device__ int fibonacci_device(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci_device(n - 1) + fibonacci_device(n - 2);
}

__global__ void fibonacci_kernel(int* output, int count) {
    for (int i = 0; i < count; ++i) {
        output[i] = fibonacci_device(i);
    }
}

void check_cuda(cudaError_t error, const char* operation) {
    if (error != cudaSuccess) {
        throw std::runtime_error(std::string("CUDA error during ") + operation +
                                 ": " + cudaGetErrorString(error));
    }
}

std::vector<int> fibonacci_gpu(int count) {
    int* output_device = nullptr;
    std::vector<int> output_host(count);

    check_cuda(cudaMalloc(&output_device, count * sizeof(int)), "device allocation");
    fibonacci_kernel<<<1, 1>>>(output_device, count);
    check_cuda(cudaGetLastError(), "kernel launch");
    check_cuda(cudaDeviceSynchronize(), "kernel synchronization");
    check_cuda(cudaMemcpy(output_host.data(), output_device, count * sizeof(int),
                          cudaMemcpyDeviceToHost),
               "device-to-host copy");
    check_cuda(cudaFree(output_device), "device deallocation");

    return output_host;
}

}  // namespace

std::vector<int> compute_cpu(int count) {
    std::vector<int> results(count);
    for (int i = 0; i < count; ++i) {
        results[i] = fibonacci_cpu(i);
    }
    return results;
}

std::vector<int> compute_gpu(int count) {
    return fibonacci_gpu(count);
}

BenchmarkResult run_benchmark(int count) {
    const auto cpu_start = std::chrono::high_resolution_clock::now();
    const std::vector<int> cpu_results = compute_cpu(count);
    const auto cpu_end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> cpu_duration = cpu_end - cpu_start;

    const auto gpu_start = std::chrono::high_resolution_clock::now();
    const std::vector<int> gpu_results = compute_gpu(count);
    const auto gpu_end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> gpu_duration = gpu_end - gpu_start;

    bool results_match = true;
    for (int i = 0; i < count; ++i) {
        if (cpu_results[i] != gpu_results[i]) {
            results_match = false;
        }
    }

    return {count, cpu_duration.count(), gpu_duration.count(), results_match};
}

}  // namespace fib
