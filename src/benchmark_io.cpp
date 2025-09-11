#include "benchmark_io.h"

#include <cctype>
#include <exception>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <unordered_map>

namespace fib {
namespace {

constexpr int kPrecision = 17;

std::string trim(std::string value) {
    while (!value.empty() &&
           std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    std::size_t start = 0;
    while (start < value.size() &&
           std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }
    return value.substr(start);
}

std::vector<std::string> split_fields(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    std::istringstream stream(line);
    while (std::getline(stream, field, ',')) {
        fields.push_back(trim(field));
    }
    if (!line.empty() && line.back() == ',') {
        fields.emplace_back();
    }
    return fields;
}

const std::string& field_at(const std::vector<std::string>& fields,
                            std::optional<std::size_t> index) {
    static const std::string empty;
    if (!index.has_value() || *index >= fields.size()) {
        return empty;
    }
    return fields[*index];
}

bool parse_int(const std::string& text, int& value) {
    try {
        std::size_t consumed = 0;
        const long parsed = std::stol(text, &consumed);
        if (consumed != text.size() ||
            parsed < std::numeric_limits<int>::min() ||
            parsed > std::numeric_limits<int>::max()) {
            return false;
        }
        value = static_cast<int>(parsed);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool parse_double(const std::string& text, double& value) {
    try {
        std::size_t consumed = 0;
        const double parsed = std::stod(text, &consumed);
        if (consumed != text.size()) {
            return false;
        }
        value = parsed;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool parse_bool(const std::string& text, bool& value) {
    if (text == "true" || text == "1") {
        value = true;
        return true;
    }
    if (text == "false" || text == "0") {
        value = false;
        return true;
    }
    return false;
}

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

bool read_csv(const std::string& path,
              std::vector<BenchmarkResult>& results,
              std::string& error) {
    std::ifstream input(path);
    if (!input) {
        error = "unable to open '" + path + "' for reading";
        return false;
    }

    std::string line;
    if (!std::getline(input, line)) {
        error = "CSV file '" + path + "' is empty";
        return false;
    }

    const std::vector<std::string> header = split_fields(line);
    std::unordered_map<std::string, std::size_t> columns;
    for (std::size_t i = 0; i < header.size(); ++i) {
        columns[header[i]] = i;
    }

    auto column = [&columns](const std::string& name) -> std::optional<std::size_t> {
        const auto found = columns.find(name);
        if (found == columns.end()) {
            return std::nullopt;
        }
        return found->second;
    };

    const std::optional<std::size_t> implementation = column("implementation");
    const std::optional<std::size_t> count = column("count");
    const std::optional<std::size_t> iterations = column("iterations");
    const std::optional<std::size_t> duration = column("duration_seconds");
    const std::optional<std::size_t> kernel = column("kernel_seconds");
    const std::optional<std::size_t> matches = column("results_match");

    if (!implementation.has_value() || !count.has_value() ||
        !duration.has_value()) {
        error = "CSV file '" + path +
                "' is missing required columns (implementation, count, "
                "duration_seconds)";
        return false;
    }

    std::vector<BenchmarkResult> loaded;
    BenchmarkResult pending;
    bool have_cpu = false;
    std::size_t line_number = 1;

    while (std::getline(input, line)) {
        ++line_number;
        if (trim(line).empty()) {
            continue;
        }
        const std::vector<std::string> fields = split_fields(line);

        BenchmarkResult row;
        row.results_match = true;
        if (!parse_int(field_at(fields, count), row.count) ||
            !parse_double(field_at(fields, duration), row.cpu_seconds)) {
            error = "invalid number on line " + std::to_string(line_number) +
                    " of '" + path + "'";
            return false;
        }
        if (iterations.has_value() &&
            !field_at(fields, iterations).empty() &&
            !parse_int(field_at(fields, iterations), row.iterations)) {
            error = "invalid iterations on line " +
                    std::to_string(line_number) + " of '" + path + "'";
            return false;
        }
        if (matches.has_value() && !field_at(fields, matches).empty() &&
            !parse_bool(field_at(fields, matches), row.results_match)) {
            error = "invalid results_match on line " +
                    std::to_string(line_number) + " of '" + path + "'";
            return false;
        }

        const std::string& kind = field_at(fields, implementation);
        if (kind == "cpu") {
            if (have_cpu) {
                error = "CPU row without matching GPU row before line " +
                        std::to_string(line_number) + " of '" + path + "'";
                return false;
            }
            pending = row;
            pending.gpu_seconds = 0.0;
            pending.gpu_kernel_seconds = 0.0;
            have_cpu = true;
        } else if (kind == "gpu") {
            if (!have_cpu || pending.count != row.count) {
                error = "GPU row without matching CPU row on line " +
                        std::to_string(line_number) + " of '" + path + "'";
                return false;
            }
            pending.gpu_seconds = row.cpu_seconds;
            pending.gpu_kernel_seconds = 0.0;
            if (kernel.has_value() && !field_at(fields, kernel).empty() &&
                !parse_double(field_at(fields, kernel),
                              pending.gpu_kernel_seconds)) {
                error = "invalid kernel_seconds on line " +
                        std::to_string(line_number) + " of '" + path + "'";
                return false;
            }
            pending.results_match = pending.results_match && row.results_match;
            loaded.push_back(pending);
            have_cpu = false;
        } else {
            error = "unknown implementation '" + kind + "' on line " +
                    std::to_string(line_number) + " of '" + path + "'";
            return false;
        }
    }

    if (have_cpu) {
        error = "missing GPU row for count " + std::to_string(pending.count) +
                " in '" + path + "'";
        return false;
    }

    results = std::move(loaded);
    return true;
}

}  // namespace fib
