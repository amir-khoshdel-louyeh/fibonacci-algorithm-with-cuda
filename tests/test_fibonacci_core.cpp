#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "fibonacci_core.h"

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

}  // namespace

int main() {
    const std::vector<int> expected = {
        0,   1,   1,    2,    3,    5,    8,    13,   21,
        34,  55,  89,   144,  233,  377,  610,  987,  1597,
        2584, 4181, 6765};

    const std::vector<int> cpu = fib::compute_cpu(expected.size());
    expect(cpu == expected, "CPU Fibonacci sequence");

    bool invalid_count_rejected = false;
    try {
        fib::run_benchmark(0);
    } catch (const std::invalid_argument&) {
        invalid_count_rejected = true;
    }
    expect(invalid_count_rejected, "invalid count is rejected");

    const fib::DeviceInfo device = fib::query_device();
    if (!device.available) {
        std::cout << "CUDA device unavailable; skipping GPU checks: "
                  << device.error << '\n';
    } else {
        expect(!device.name.empty(), "device name is reported");
        expect(device.compute_major > 0, "compute capability is reported");

        const std::vector<int> gpu = fib::compute_gpu(expected.size());
        expect(gpu == expected, "GPU Fibonacci sequence");

        const fib::BenchmarkResult result =
            fib::run_benchmark(10, fib::BenchmarkOptions{1, 0});
        expect(result.count == 10, "benchmark count");
        expect(result.iterations == 1, "benchmark iterations");
        expect(result.results_match, "benchmark results match");
        expect(result.cpu_seconds > 0.0, "CPU duration is positive");
        expect(result.gpu_seconds > 0.0, "GPU duration is positive");
        expect(result.gpu_kernel_seconds > 0.0, "kernel duration is positive");
    }

    if (failures != 0) {
        std::cerr << failures << " assertion(s) failed\n";
        return 1;
    }
    std::cout << "Fibonacci core tests passed\n";
    return 0;
}
