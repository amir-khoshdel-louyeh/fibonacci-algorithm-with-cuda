%%writefile app.cu

#include <iostream>
#include <cuda_runtime.h>
#include <chrono>

// ---------------------------
// Recursive CPU Fibonacci
int fibonacci_cpu(int n) {
    if (n <= 1) return n;
    return fibonacci_cpu(n - 1) + fibonacci_cpu(n - 2);
}

// ---------------------------
// Recursive GPU Fibonacci
__device__ int fibonacci_device(int n) {
    if (n <= 1) return n;
    return fibonacci_device(n - 1) + fibonacci_device(n - 2);
}

// ---------------------------
// CUDA kernel with single thread (sequential execution)
__global__ void fibonacci_kernel(int* output, int count) {
    for (int i = 0; i < count; ++i) {
        output[i] = fibonacci_device(i);
    }
}

// ---------------------------
// Error checking helper
void checkCuda(cudaError_t err, const char* msg) {
    if (err != cudaSuccess) {
        std::cerr << "CUDA Error: " << msg << " -> " << cudaGetErrorString(err) << "\n";
        exit(1);
    }
}

// ---------------------------
// GPU launcher
void fibonacci_gpu_sequential(int count, int* output_host) {
    int* output_device;

    checkCuda(cudaMalloc(&output_device, count * sizeof(int)), "Alloc output");
    
    // Launch only 1 thread for sequential computation
    fibonacci_kernel<<<1, 1>>>(output_device, count);
    checkCuda(cudaGetLastError(), "Kernel launch");
    checkCuda(cudaDeviceSynchronize(), "Kernel sync");

    checkCuda(cudaMemcpy(output_host, output_device, count * sizeof(int), cudaMemcpyDeviceToHost), "Copy output");

    cudaFree(output_device);
}

// ---------------------------
// Main function
int main() {
    const int N =5;  // Keep small due to recursive inefficiency

    int* result_cpu = new int[N];
    int* result_gpu = new int[N];

    // CPU timing
    auto start_cpu = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        result_cpu[i] = fibonacci_cpu(i);
    }
    auto end_cpu = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> cpu_duration = end_cpu - start_cpu;
    std::cout << "CPU Fibonacci time: " << cpu_duration.count() << " s\n";

    // GPU timing
    auto start_gpu = std::chrono::high_resolution_clock::now();
    fibonacci_gpu_sequential(N, result_gpu);
    auto end_gpu = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> gpu_duration = end_gpu - start_gpu;
    std::cout << "GPU Fibonacci time: " << gpu_duration.count() << " s\n";

    // Compare results
    bool equal = true;
    for (int i = 0; i < N; ++i) {
        if (result_cpu[i] != result_gpu[i]) {
            equal = false;
            std::cout << "Mismatch at index " << i << ": CPU=" << result_cpu[i] << ", GPU=" << result_gpu[i] << "\n";
        }
    }

    std::cout << "Fibonacci results match? " << (equal ? "Yes" : "No") << "\n";

    delete[] result_cpu;
    delete[] result_gpu;
    return 0;
}
