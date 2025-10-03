#include "fibonacci_core.h"

#include <algorithm>
#include <chrono>
#include <cuda_runtime.h>
#include <stdexcept>
#include <string>
#include <vector>

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

double median(std::vector<double> samples) {
    std::sort(samples.begin(), samples.end());
    const std::size_t middle = samples.size() / 2;
    if (samples.size() % 2 == 0) {
        return (samples[middle - 1] + samples[middle]) / 2.0;
    }
    return samples[middle];
}

std::vector<int> fibonacci_gpu(int count, double& kernel_seconds) {
    int* output_device = nullptr;
    std::vector<int> output_host(count);
    cudaEvent_t kernel_start = nullptr;
    cudaEvent_t kernel_stop = nullptr;
    float elapsed_milliseconds = 0.0f;

    check_cuda(cudaEventCreate(&kernel_start), "event creation");
    check_cuda(cudaEventCreate(&kernel_stop), "event creation");
    check_cuda(cudaMalloc(&output_device, count * sizeof(int)), "device allocation");

    check_cuda(cudaEventRecord(kernel_start), "event recording");
    fibonacci_kernel<<<1, 1>>>(output_device, count);
    check_cuda(cudaGetLastError(), "kernel launch");
    check_cuda(cudaEventRecord(kernel_stop), "event recording");
    check_cuda(cudaEventSynchronize(kernel_stop), "kernel synchronization");
    check_cuda(cudaEventElapsedTime(&elapsed_milliseconds, kernel_start, kernel_stop),
               "event timing");
    kernel_seconds = elapsed_milliseconds / 1000.0;

    check_cuda(cudaMemcpy(output_host.data(), output_device, count * sizeof(int),
                          cudaMemcpyDeviceToHost),
               "device-to-host copy");
    check_cuda(cudaFree(output_device), "device deallocation");
    check_cuda(cudaEventDestroy(kernel_start), "event destruction");
    check_cuda(cudaEventDestroy(kernel_stop), "event destruction");

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
    double kernel_seconds = 0.0;
    return fibonacci_gpu(count, kernel_seconds);
}

DeviceInfo query_device() {
    DeviceInfo info;
    int device_count = 0;
    cudaError_t status = cudaGetDeviceCount(&device_count);
    if (status != cudaSuccess) {
        info.error = cudaGetErrorString(status);
        return info;
    }
    if (device_count == 0) {
        info.error = "no CUDA-capable device found";
        return info;
    }

    cudaDeviceProp properties{};
    status = cudaGetDeviceProperties(&properties, 0);
    if (status != cudaSuccess) {
        info.error = cudaGetErrorString(status);
        return info;
    }

    info.available = true;
    info.name = properties.name;
    info.compute_major = properties.major;
    info.compute_minor = properties.minor;
    info.multiprocessor_count = properties.multiProcessorCount;
    info.total_memory_bytes = properties.totalGlobalMem;
    return info;
}

BenchmarkResult run_benchmark(int count, const BenchmarkOptions& options) {
    if (count < 1 || count > kMaxCount) {
        throw std::invalid_argument("benchmark count out of range");
    }
    if (options.iterations < 1) {
        throw std::invalid_argument("benchmark iterations must be at least 1");
    }
    if (options.warmup_iterations < 0) {
        throw std::invalid_argument("benchmark warmup iterations must not be negative");
    }

    std::vector<int> cpu_results;
    for (int i = 0; i < options.warmup_iterations; ++i) {
        cpu_results = compute_cpu(count);
    }
    std::vector<double> cpu_samples;
    cpu_samples.reserve(options.iterations);
    for (int i = 0; i < options.iterations; ++i) {
        const auto start = std::chrono::high_resolution_clock::now();
        cpu_results = compute_cpu(count);
        const auto end = std::chrono::high_resolution_clock::now();
        cpu_samples.push_back(std::chrono::duration<double>(end - start).count());
    }

    std::vector<int> gpu_results;
    double kernel_seconds = 0.0;
    for (int i = 0; i < options.warmup_iterations; ++i) {
        gpu_results = fibonacci_gpu(count, kernel_seconds);
    }
    std::vector<double> gpu_samples;
    std::vector<double> gpu_kernel_samples;
    gpu_samples.reserve(options.iterations);
    gpu_kernel_samples.reserve(options.iterations);
    for (int i = 0; i < options.iterations; ++i) {
        const auto start = std::chrono::high_resolution_clock::now();
        gpu_results = fibonacci_gpu(count, kernel_seconds);
        const auto end = std::chrono::high_resolution_clock::now();
        gpu_samples.push_back(std::chrono::duration<double>(end - start).count());
        gpu_kernel_samples.push_back(kernel_seconds);
    }

    return {count,
            options.iterations,
            median(cpu_samples),
            median(gpu_samples),
            median(gpu_kernel_samples),
            cpu_results == gpu_results};
}

}  // namespace fib
