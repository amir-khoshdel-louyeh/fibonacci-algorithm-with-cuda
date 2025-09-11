#include "benchmark_io.h"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace fib {
namespace {

constexpr int kPrecision = 17;

bool write_text_file(const std::string& path,
                     const std::string& contents,
                     std::string& error) {
    std::ofstream output(path);
    if (!output) {
        error = "unable to open '" + path + "' for writing";
        return false;
    }
    output << contents;
    if (!output) {
        error = "failed while writing '" + path + "'";
        return false;
    }
    return true;
}

}  // namespace

std::string results_to_csv(const std::vector<BenchmarkResult>& results) {
    std::ostringstream output;
    output << "schema_version,implementation,count,iterations,"
              "duration_seconds,kernel_seconds,results_match\n";
    output << std::setprecision(kPrecision);
    for (const BenchmarkResult& result : results) {
        const char* match = result.results_match ? "true" : "false";
        output << kDataSchemaVersion << ",cpu," << result.count << ','
               << result.iterations << ',' << result.cpu_seconds << ",,"
               << match << '\n'
               << kDataSchemaVersion << ",gpu," << result.count << ','
               << result.iterations << ',' << result.gpu_seconds << ','
               << result.gpu_kernel_seconds << ',' << match << '\n';
    }
    return output.str();
}

std::string results_to_json(const std::vector<BenchmarkResult>& results) {
    std::ostringstream output;
    output << std::setprecision(kPrecision);
    output << "{\n"
           << "  \"schema_version\": " << kDataSchemaVersion << ",\n"
           << "  \"results\": [\n";
    for (std::size_t i = 0; i < results.size(); ++i) {
        const BenchmarkResult& result = results[i];
        output << "    {\n"
               << "      \"count\": " << result.count << ",\n"
               << "      \"iterations\": " << result.iterations << ",\n"
               << "      \"cpu_seconds\": " << result.cpu_seconds << ",\n"
               << "      \"gpu_seconds\": " << result.gpu_seconds << ",\n"
               << "      \"gpu_kernel_seconds\": " << result.gpu_kernel_seconds
               << ",\n"
               << "      \"results_match\": "
               << (result.results_match ? "true" : "false") << "\n"
               << "    }" << (i + 1 < results.size() ? "," : "") << '\n';
    }
    output << "  ]\n"
           << "}\n";
    return output.str();
}

bool write_csv(const std::string& path,
               const std::vector<BenchmarkResult>& results,
               std::string& error) {
    return write_text_file(path, results_to_csv(results), error);
}

bool write_json(const std::string& path,
                const std::vector<BenchmarkResult>& results,
                std::string& error) {
    return write_text_file(path, results_to_json(results), error);
}

}  // namespace fib
