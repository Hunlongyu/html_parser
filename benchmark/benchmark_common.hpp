#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace bench {

namespace fs = std::filesystem;

constexpr std::size_t KIB = 1024;
constexpr std::size_t MIB = 1024 * 1024;

struct Stats {
    std::size_t samples{};
    double      avg_ms{};
    double      min_ms{};
    double      max_ms{};
    double      stddev_ms{};
    double      p50_ms{};
    double      p95_ms{};
};

inline auto compute_stats(const std::vector<double>& durations_ms) -> Stats {
    if (durations_ms.empty()) {
        throw std::runtime_error("Cannot compute stats from an empty sample set");
    }

    Stats stats;
    stats.samples = durations_ms.size();

    const double total_ms = std::accumulate(durations_ms.begin(), durations_ms.end(), 0.0);
    stats.avg_ms          = total_ms / static_cast<double>(durations_ms.size());

    const auto min_max = std::minmax_element(durations_ms.begin(), durations_ms.end());
    stats.min_ms       = *min_max.first;
    stats.max_ms       = *min_max.second;

    double variance = 0.0;
    for (const double sample_ms : durations_ms) {
        const double delta = sample_ms - stats.avg_ms;
        variance += delta * delta;
    }
    variance /= static_cast<double>(durations_ms.size());
    stats.stddev_ms = std::sqrt(variance);

    std::vector<double> sorted = durations_ms;
    std::sort(sorted.begin(), sorted.end());

    const auto percentile = [&sorted](const double p) {
        const double scaled_index = std::ceil(p * static_cast<double>(sorted.size()));
        std::size_t  index        = static_cast<std::size_t>(scaled_index > 0.0 ? scaled_index - 1.0 : 0.0);
        if (index >= sorted.size()) {
            index = sorted.size() - 1;
        }
        return sorted[index];
    };

    stats.p50_ms = percentile(0.50);
    stats.p95_ms = percentile(0.95);
    return stats;
}

inline auto throughput_mib_s(const std::size_t input_bytes, const double avg_ms) -> double {
    if (input_bytes == 0 || avg_ms <= 0.0) {
        return 0.0;
    }

    return (static_cast<double>(input_bytes) / static_cast<double>(MIB)) / (avg_ms / 1000.0);
}

inline auto csv_escape(std::string_view text) -> std::string {
    bool needs_quotes = false;
    for (const char ch : text) {
        if (ch == ',' || ch == '"' || ch == '\n' || ch == '\r') {
            needs_quotes = true;
            break;
        }
    }

    if (!needs_quotes) {
        return std::string(text);
    }

    std::string escaped;
    escaped.reserve(text.size() + 2);
    escaped.push_back('"');
    for (const char ch : text) {
        if (ch == '"') {
            escaped.push_back('"');
        }
        escaped.push_back(ch);
    }
    escaped.push_back('"');
    return escaped;
}

inline void print_csv_header() {
    std::cout
        << "target,category,scenario,input_bytes,iterations,result_count,avg_ms,min_ms,max_ms,stddev_ms,p50_ms,p95_ms,throughput_mib_s\n";
}

inline void print_csv_row(
    std::string_view target,
    std::string_view category,
    std::string_view scenario,
    const std::size_t input_bytes,
    const int iterations,
    const std::size_t result_count,
    const Stats& stats,
    const double throughput) {
    const auto old_flags     = std::cout.flags();
    const auto old_precision = std::cout.precision();

    std::cout << std::fixed << std::setprecision(6)
              << csv_escape(target) << ','
              << csv_escape(category) << ','
              << csv_escape(scenario) << ','
              << input_bytes << ','
              << iterations << ','
              << result_count << ','
              << stats.avg_ms << ','
              << stats.min_ms << ','
              << stats.max_ms << ','
              << stats.stddev_ms << ','
              << stats.p50_ms << ','
              << stats.p95_ms << ','
              << throughput << '\n';

    std::cout.flags(old_flags);
    std::cout.precision(old_precision);
}

inline auto read_binary_file(const fs::path& path) -> std::string {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

inline auto example_html_root() -> fs::path {
#ifdef HPS_SOURCE_DIR
    return fs::path(HPS_SOURCE_DIR) / "examples" / "html";
#else
    return fs::current_path() / "examples" / "html";
#endif
}

inline auto example_html_files() -> std::vector<fs::path> {
    const fs::path root = example_html_root();
    const std::vector<std::string_view> preferred_files = {
        "base.html",
        "complex.html",
        "fofa.html",
        "panlong.html",
        "shodan.html",
    };

    std::vector<fs::path> files;
    files.reserve(preferred_files.size());
    for (const auto name : preferred_files) {
        const fs::path candidate = root / name;
        if (fs::exists(candidate)) {
            files.push_back(candidate);
        }
    }

    return files;
}

inline auto recommended_iterations(const std::size_t input_bytes) -> int {
    if (input_bytes < 10 * KIB) {
        return 400;
    }
    if (input_bytes < 100 * KIB) {
        return 200;
    }
    if (input_bytes < MIB) {
        return 60;
    }
    return 12;
}

inline auto recommended_query_iterations(const std::size_t input_bytes) -> int {
    if (input_bytes < 100 * KIB) {
        return 320;
    }
    if (input_bytes < MIB) {
        return 160;
    }
    return 48;
}

}  // namespace bench