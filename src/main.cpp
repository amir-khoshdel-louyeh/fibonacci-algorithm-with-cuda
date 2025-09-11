#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "fibonacci_core.h"

namespace {

struct Options {
    int count = fib::kDefaultCount;
    int sweep_start = 0;
    int sweep_end = 0;
    fib::BenchmarkOptions benchmark;
    std::string csv_path;
};

void print_usage(const char* executable) {
    std::cout << "Usage: " << executable
              << " [--count N] [--csv PATH]\n\n"
              << "Options:\n"
              << "  --count N   Number of Fibonacci terms to calculate (1-"
              << fib::kMaxCount << ", default " << fib::kDefaultCount << ")\n"
              << "  --sweep A B Benchmark every term count from A through B\n"
              << "  --iterations N  Measured runs per implementation, median is "
                 "reported (default 3)\n"
              << "  --warmup N  Unmeasured runs before timing (default 1)\n"
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

bool parse_non_negative_int(const std::string& value, int& result) {
    try {
        std::size_t parsed_length = 0;
        const long parsed = std::stol(value, &parsed_length);
        if (parsed_length != value.size() ||
            parsed < 0 ||
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
                options.count > fib::kMaxCount) {
                std::cerr << "Error: --count must be between 1 and "
                          << fib::kMaxCount << ".\n";
                return false;
            }
            continue;
        }
        if (argument == "--sweep" && i + 2 < argc) {
            if (!parse_positive_int(argv[++i], options.sweep_start) ||
                !parse_positive_int(argv[++i], options.sweep_end) ||
                options.sweep_start > fib::kMaxCount ||
                options.sweep_end > fib::kMaxCount ||
                options.sweep_start > options.sweep_end) {
                std::cerr << "Error: --sweep values must be between 1 and "
                          << fib::kMaxCount << ", with the start no greater than "
                          << "the end.\n";
                return false;
            }
            continue;
        }
        if (argument == "--iterations" && i + 1 < argc) {
            if (!parse_positive_int(argv[++i], options.benchmark.iterations)) {
                std::cerr << "Error: --iterations must be at least 1.\n";
                return false;
            }
            continue;
        }
        if (argument == "--warmup" && i + 1 < argc) {
            if (!parse_non_negative_int(argv[++i],
                                        options.benchmark.warmup_iterations)) {
                std::cerr << "Error: --warmup must not be negative.\n";
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

void print_result(const fib::BenchmarkResult& result) {
    std::cout << "Fibonacci benchmark (" << result.count << " terms)\n"
              << "Iterations: " << result.iterations << " (median)\n"
              << "CPU time: " << std::fixed << std::setprecision(9)
              << result.cpu_seconds << " s\n"
              << "GPU time: " << result.gpu_seconds << " s\n"
              << "GPU kernel time: " << result.gpu_kernel_seconds << " s\n"
              << "Results match: " << (result.results_match ? "Yes" : "No")
              << '\n';
}

bool write_csv(const std::string& path,
               const std::vector<fib::BenchmarkResult>& results) {
    std::ofstream output(path);
    if (!output) {
        std::cerr << "Error: unable to open CSV output '" << path << "'.\n";
        return false;
    }

    output << "implementation,count,duration_seconds,results_match\n";
    for (const fib::BenchmarkResult& result : results) {
        output << "cpu," << result.count << ',' << std::setprecision(12)
               << result.cpu_seconds << ','
               << (result.results_match ? "true" : "false") << '\n'
               << "gpu," << result.count << ',' << std::setprecision(12)
               << result.gpu_seconds << ','
               << (result.results_match ? "true" : "false") << '\n';
    }
    return true;
}

int run(const Options& options) {
    std::vector<fib::BenchmarkResult> results;
    if (options.sweep_start != 0) {
        for (int count = options.sweep_start; count <= options.sweep_end; ++count) {
            results.push_back(fib::run_benchmark(count, options.benchmark));
        }
        std::cout << "Fibonacci benchmark sweep\n"
                  << "Count       CPU (s)        GPU (s)        Match\n";
        for (const fib::BenchmarkResult& result : results) {
            std::cout << std::left << std::setw(12) << result.count
                      << std::setw(15) << std::fixed << std::setprecision(9)
                      << result.cpu_seconds << std::setw(15)
                      << result.gpu_seconds << (result.results_match ? "Yes" : "No")
                      << '\n';
        }
    } else {
        results.push_back(fib::run_benchmark(options.count, options.benchmark));
        print_result(results.front());
    }

    if (!options.csv_path.empty() && !write_csv(options.csv_path, results)) {
        return 1;
    }
    if (!options.csv_path.empty()) {
        std::cout << "CSV written to: " << options.csv_path << '\n';
    }

    for (const fib::BenchmarkResult& result : results) {
        if (!result.results_match) {
            return 1;
        }
    }
    return 0;
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

    try {
        return run(options);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
