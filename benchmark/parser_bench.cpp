#include "benchmark_common.hpp"
#include "hps/core/document.hpp"
#include "hps/parsing/html_parser.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <vector>

using namespace hps;
namespace fs = std::filesystem;

namespace {

auto count_nodes(const Node& node) -> std::size_t {
    std::size_t total = 1;
    for (auto child = node.first_child(); child; child = child->next_sibling()) {
        total += count_nodes(*child);
    }
    return total;
}

}  // namespace

int main() {
    try {
        const auto files = bench::example_html_files();
        if (files.empty()) {
            std::cerr << "Error: no example HTML files found under " << bench::example_html_root() << std::endl;
            return 1;
        }

        bench::print_csv_header();

        Options options = Options::performance();

        for (const fs::path& file_path : files) {
            const std::string source = bench::read_binary_file(file_path);
            const int         iterations = bench::recommended_iterations(source.size());

            HTMLParser warmup_parser;
            const auto warmup_doc = warmup_parser.parse(source, options);
            if (!warmup_doc) {
                std::cerr << "Error: warmup parse failed for " << file_path << std::endl;
                return 1;
            }

            const std::size_t node_count = count_nodes(*warmup_doc);
            std::vector<double> durations_ms;
            durations_ms.reserve(static_cast<std::size_t>(iterations));

            HTMLParser parser;
            for (int iteration = 0; iteration < iterations; ++iteration) {
                const auto start = std::chrono::steady_clock::now();
                const auto parsed_doc = parser.parse(source, options);
                const auto end = std::chrono::steady_clock::now();

                if (!parsed_doc) {
                    std::cerr << "Error: parse failed during benchmark for " << file_path << std::endl;
                    return 1;
                }

                const std::chrono::duration<double, std::milli> elapsed_ms = end - start;
                durations_ms.push_back(elapsed_ms.count());
            }

            const auto stats      = bench::compute_stats(durations_ms);
            const auto throughput = bench::throughput_mib_s(source.size(), stats.avg_ms);
            bench::print_csv_row(
                "parser_bench",
                "parse",
                file_path.filename().string(),
                source.size(),
                iterations,
                node_count,
                stats,
                throughput);
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
