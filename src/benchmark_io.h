#pragma once

#include <string>
#include <vector>

#include "fibonacci_core.h"

namespace fib {

inline constexpr int kDataSchemaVersion = 1;

std::string results_to_csv(const std::vector<BenchmarkResult>& results);
std::string results_to_json(const std::vector<BenchmarkResult>& results);

bool write_csv(const std::string& path,
               const std::vector<BenchmarkResult>& results,
               std::string& error);
bool write_json(const std::string& path,
                const std::vector<BenchmarkResult>& results,
                std::string& error);

bool read_csv(const std::string& path,
              std::vector<BenchmarkResult>& results,
              std::string& error);

}  // namespace fib
