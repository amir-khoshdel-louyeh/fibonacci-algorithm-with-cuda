#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "benchmark_io.h"

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool nearly_equal(double left, double right) {
    const double scale = std::max({1.0, std::abs(left), std::abs(right)});
    return std::abs(left - right) <= 1e-12 * scale;
}

void expect_equal(const fib::BenchmarkResult& actual,
                  const fib::BenchmarkResult& expected) {
    expect(actual.count == expected.count, "count");
    expect(actual.iterations == expected.iterations, "iterations");
    expect(nearly_equal(actual.cpu_seconds, expected.cpu_seconds),
           "cpu_seconds");
    expect(nearly_equal(actual.gpu_seconds, expected.gpu_seconds),
           "gpu_seconds");
    expect(nearly_equal(actual.gpu_kernel_seconds,
                        expected.gpu_kernel_seconds),
           "gpu_kernel_seconds");
    expect(actual.results_match == expected.results_match, "results_match");
}

}  // namespace

int main() {
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path();
    const std::filesystem::path csv_path =
        directory / "fibonacci_benchmark_io_test.csv";
    const std::filesystem::path legacy_path =
        directory / "fibonacci_benchmark_io_legacy.csv";

    const std::vector<fib::BenchmarkResult> original = {
        {5, 3, 1.5e-6, 2.0e-4, 1.0e-5, true},
        {10, 2, 3.0e-5, 5.0e-4, 2.0e-5, false},
    };

    std::string error;
    expect(fib::write_csv(csv_path.string(), original, error),
           "write_csv: " + error);

    std::vector<fib::BenchmarkResult> loaded;
    expect(fib::read_csv(csv_path.string(), loaded, error),
           "read_csv: " + error);
    expect(loaded.size() == original.size(), "round-trip row count");
    if (loaded.size() == original.size()) {
        for (std::size_t i = 0; i < loaded.size(); ++i) {
            expect_equal(loaded[i], original[i]);
        }
    }

    {
        std::ofstream legacy(legacy_path);
        legacy << "implementation,count,duration_seconds,results_match\n"
               << "cpu,5,0.000001,true\n"
               << "gpu,5,0.0004,true\n";
    }
    loaded.clear();
    expect(fib::read_csv(legacy_path.string(), loaded, error),
           "read_csv legacy: " + error);
    expect(loaded.size() == 1, "legacy row count");
    if (loaded.size() == 1) {
        expect(loaded[0].count == 5, "legacy count");
        expect(loaded[0].iterations == 0, "legacy iterations default");
        expect(nearly_equal(loaded[0].cpu_seconds, 0.000001),
               "legacy cpu duration");
        expect(nearly_equal(loaded[0].gpu_seconds, 0.0004),
               "legacy gpu duration");
        expect(loaded[0].results_match, "legacy match");
    }

    std::filesystem::remove(csv_path);
    std::filesystem::remove(legacy_path);

    if (failures != 0) {
        std::cerr << failures << " assertion(s) failed\n";
        return 1;
    }
    std::cout << "benchmark IO tests passed\n";
    return 0;
}
