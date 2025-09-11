#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include <cuda_runtime.h>

namespace {

constexpr int kDefaultCount = 5;
constexpr int kMaxCount = 47;

struct Options {
    int count = kDefaultCount;
    std::string csv_path;
};

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
        std::cerr << "CUDA error during " << operation << ": "
                  << cudaGetErrorString(error) << '\n';
        std::exit(EXIT_FAILURE);
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

void print_usage(const char* executable) {
    std::cout << "Usage: " << executable
              << " [--count N] [--csv PATH]\n\n"
              << "Options:\n"
              << "  --count N   Number of Fibonacci terms to calculate (1-"
              << kMaxCount << ", default " << kDefaultCount << ")\n"
              << "  --csv PATH  Write benchmark timing data as CSV\n"
              << "  --help      Show this help message\n";
}

bool parse_positive_int(const std::string& value, int& result) {
    try {
        std::size_t parsed_length = 0;
        const long parsed = std::stol(value, &parsed_length);
        if (parsed_length != value.size() ||
            parsed < 1 ||
            parsed > std::numeric_limits<int>::max()) {
            return false;
        }
        result = static_cast<int>(parsed);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool parse_options(int argc, char** argv, Options& options, bool& show_help) {
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--help") {
            print_usage(argv[0]);
            show_help = true;
            return true;
        }
        if (argument == "--count" && i + 1 < argc) {
            if (!parse_positive_int(argv[++i], options.count) ||
                options.count > kMaxCount) {
                std::cerr << "Error: --count must be between 1 and "
                          << kMaxCount << ".\n";
                return false;
            }
            continue;
        }
        if (argument == "--csv" && i + 1 < argc) {
            options.csv_path = argv[++i];
            continue;
        }

        std::cerr << "Error: unknown or incomplete option '" << argument << "'.\n";
        print_usage(argv[0]);
        return false;
    }
    return true;
}

bool write_csv(const std::string& path, int count, double cpu_seconds,
               double gpu_seconds) {
    std::ofstream output(path);
    if (!output) {
        std::cerr << "Error: unable to open CSV output '" << path << "'.\n";
        return false;
    }

    output << "implementation,count,duration_seconds\n"
           << "cpu," << count << ',' << std::setprecision(12) << cpu_seconds
           << '\n'
           << "gpu," << count << ',' << std::setprecision(12) << gpu_seconds
           << '\n';
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    Options options;
    bool show_help = false;
    if (!parse_options(argc, argv, options, show_help)) {
        return 1;
    }
    if (show_help) {
        return 0;
    }

    std::vector<int> cpu_results(options.count);
    const auto cpu_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < options.count; ++i) {
        cpu_results[i] = fibonacci_cpu(i);
    }
    const auto cpu_end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> cpu_duration = cpu_end - cpu_start;

    const auto gpu_start = std::chrono::high_resolution_clock::now();
    const std::vector<int> gpu_results = fibonacci_gpu(options.count);
    const auto gpu_end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> gpu_duration = gpu_end - gpu_start;

    bool results_match = true;
    for (int i = 0; i < options.count; ++i) {
        if (cpu_results[i] != gpu_results[i]) {
            results_match = false;
            std::cout << "Mismatch at index " << i << ": CPU=" << cpu_results[i]
                      << ", GPU=" << gpu_results[i] << '\n';
        }
    }

    std::cout << "Fibonacci benchmark (" << options.count << " terms)\n"
              << "CPU time: " << std::fixed << std::setprecision(9)
              << cpu_duration.count() << " s\n"
              << "GPU time: " << gpu_duration.count() << " s\n"
              << "Results match: " << (results_match ? "Yes" : "No") << '\n';

    if (!options.csv_path.empty() &&
        !write_csv(options.csv_path, options.count, cpu_duration.count(),
                   gpu_duration.count())) {
        return 1;
    }
    if (!options.csv_path.empty()) {
        std::cout << "CSV written to: " << options.csv_path << '\n';
    }

    return results_match ? 0 : 1;
}
