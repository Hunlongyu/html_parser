#include "benchmark_common.hpp"
#include "hps/parsing/tokenizer.hpp"

#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

using namespace hps;

namespace {

auto generate_html_for_target_bytes(const std::size_t target_bytes) -> std::string {
    const std::string header = "<!DOCTYPE html><html><head><title>Tokenizer Benchmark</title></head><body>";
    const std::string footer = "</body></html>";
    const std::string block  = R"(
        <article class="entry" data-id="42">
            <header>
                <h1>Tokenizer Benchmark</h1>
                <time datetime="2026-04-08">2026-04-08</time>
            </header>
            <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit.</p>
            <ul>
                <li>One</li>
                <li>Two</li>
                <li>Three</li>
            </ul>
            <img src="image.jpg" alt="preview" width="320" height="180" />
            <!-- synthetic benchmark payload -->
        </article>
    )";

    const std::size_t payload_budget =
        target_bytes > (header.size() + footer.size()) ? target_bytes - header.size() - footer.size() : block.size();
    const std::size_t repeat_count = std::max<std::size_t>(1, (payload_budget + block.size() - 1) / block.size());

    std::string html;
    html.reserve(header.size() + footer.size() + repeat_count * block.size());
    html += header;
    for (std::size_t i = 0; i < repeat_count; ++i) {
        html += block;
    }
    html += footer;
    return html;
}

}  // namespace

int main() {
    const std::array<std::pair<std::string_view, std::size_t>, 4> scenarios = {{
        {"synthetic_8k", 8 * bench::KIB},
        {"synthetic_64k", 64 * bench::KIB},
        {"synthetic_512k", 512 * bench::KIB},
        {"synthetic_2048k", 2 * bench::MIB},
    }};

    bench::print_csv_header();

    for (const auto& [scenario_name, target_bytes] : scenarios) {
        const std::string source = generate_html_for_target_bytes(target_bytes);
        const int         iterations = bench::recommended_iterations(source.size());

        Tokenizer warmup_tokenizer(source, Options::performance());
        const auto warmup_tokens = warmup_tokenizer.tokenize_all();
        const std::size_t token_count = warmup_tokens.size();

        std::vector<double> durations_ms;
        durations_ms.reserve(static_cast<std::size_t>(iterations));

        for (int iteration = 0; iteration < iterations; ++iteration) {
            const auto start = std::chrono::steady_clock::now();
            Tokenizer  tokenizer(source, Options::performance());
            const auto tokens = tokenizer.tokenize_all();
            const auto end = std::chrono::steady_clock::now();

            if (tokens.size() != token_count) {
                std::cerr << "Tokenizer result drift detected in scenario " << scenario_name << '\n';
                return 1;
            }

            const std::chrono::duration<double, std::milli> elapsed_ms = end - start;
            durations_ms.push_back(elapsed_ms.count());
        }

        const auto stats      = bench::compute_stats(durations_ms);
        const auto throughput = bench::throughput_mib_s(source.size(), stats.avg_ms);
        bench::print_csv_row(
            "tokenizer_bench",
            "tokenize",
            scenario_name,
            source.size(),
            iterations,
            token_count,
            stats,
            throughput);
    }

    return 0;
}
