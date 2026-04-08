#include "benchmark_common.hpp"
#include "hps/parsing/html_parser.hpp"
#include "hps/query/css/css_parser.hpp"
#include "hps/query/css/css_matcher.hpp"

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace hps;

namespace {

auto generate_html_for_target_bytes(const std::size_t target_bytes) -> std::string {
    const std::string header = "<!DOCTYPE html><html><head><title>CSS Benchmark</title></head><body>";
    const std::string footer = "</body></html>";
    const std::string block  = R"(
        <div class="container wrapper" id="main-content">
            <header class="header">
                <h1 id="logo">Logo</h1>
                <nav>
                    <ul>
                        <li><a href="#" class="nav-link active">Home</a></li>
                        <li><a href="#" class="nav-link">About</a></li>
                        <li><a href="#" class="nav-link">Contact</a></li>
                    </ul>
                </nav>
            </header>
            <main>
                <section class="section feature">
                    <h2>Feature 1</h2>
                    <p class="description">Description text for feature 1.</p>
                    <button class="btn btn-primary" disabled>Action</button>
                </section>
                <section class="section list">
                    <ul>
                        <li>Item 1</li>
                        <li>Item 2</li>
                        <li>Item 3</li>
                        <li>Item 4</li>
                        <li>Item 5</li>
                    </ul>
                </section>
                <div class="complex-structure">
                    <span data-info="test">Info</span>
                    <input type="text" name="username" value="user" />
                    <input type="checkbox" checked />
                </div>
            </main>
            <footer>
                <p>&copy; 2023 Benchmark Corp.</p>
            </footer>
        </div>
    )";

    const std::size_t payload_budget =
        target_bytes > (header.size() + footer.size()) ? target_bytes - header.size() - footer.size() : block.size();
    const std::size_t repeat_count = std::max<std::size_t>(1, (payload_budget + block.size() - 1) / block.size());

    std::string html;
    html.reserve(header.size() + footer.size() + repeat_count * block.size());
    html += header;
    for (std::size_t i = 0; i < repeat_count; ++i) {
        html += "<div id='wrapper-" + std::to_string(i) + "'>";
        html += block;
        html += "</div>";
    }

    html += footer;
    return html;
}

}  // namespace

int main() {
    const std::string html = generate_html_for_target_bytes(512 * bench::KIB);

    HTMLParser html_parser;
    const auto document = html_parser.parse(html, Options::performance());
    if (!document) {
        std::cerr << "Failed to parse HTML" << std::endl;
        return 1;
    }

    const std::array<std::string_view, 14> selectors = {
        "div",
        ".container",
        "#main-content",
        "[type='text']",
        "a[href='#']",
        "div > p",
        "ul li a",
        "h1 + nav",
        "li:first-child",
        "li:nth-child(2)",
        "input:checked",
        "button:disabled",
        "div.container > header nav ul li a.active",
        "section.section h2 + p.description",
    };

    bench::print_csv_header();

    const int parse_iterations = 500;
    std::vector<double> parse_durations_ms;
    parse_durations_ms.reserve(static_cast<std::size_t>(parse_iterations));
    std::size_t selector_bytes = 0;
    for (const auto selector : selectors) {
        selector_bytes += selector.size();
    }

    for (int iteration = 0; iteration < parse_iterations; ++iteration) {
        const auto start = std::chrono::steady_clock::now();
        for (const auto selector_text : selectors) {
            CSSParser parser(selector_text);
            const auto selector = parser.parse_selector_list();
            if (!selector) {
                std::cerr << "Failed to parse selector: " << selector_text << std::endl;
                return 1;
            }
        }
        const auto end = std::chrono::steady_clock::now();
        const std::chrono::duration<double, std::milli> elapsed_ms = end - start;
        parse_durations_ms.push_back(elapsed_ms.count());
    }

    const auto parse_stats = bench::compute_stats(parse_durations_ms);
    bench::print_csv_row(
        "css_selector_bench",
        "parse_selector_set",
        "selector_set",
        selector_bytes,
        parse_iterations,
        selectors.size(),
        parse_stats,
        bench::throughput_mib_s(selector_bytes, parse_stats.avg_ms));

    const int match_iterations = bench::recommended_query_iterations(html.size());
    for (const auto selector_text : selectors) {
        CSSParser parser(selector_text);
        auto      selector = parser.parse_selector_list();
        if (!selector) {
            std::cerr << "Failed to prepare selector: " << selector_text << std::endl;
            return 1;
        }

        const auto warmup_results = CSSMatcher::find_all(*document, *selector);
        const std::size_t match_count = warmup_results.size();

        std::vector<double> durations_ms;
        durations_ms.reserve(static_cast<std::size_t>(match_iterations));
        for (int iteration = 0; iteration < match_iterations; ++iteration) {
            const auto start = std::chrono::steady_clock::now();
            const auto results = CSSMatcher::find_all(*document, *selector);
            const auto end = std::chrono::steady_clock::now();

            if (results.size() != match_count) {
                std::cerr << "Selector result drift detected for: " << selector_text << std::endl;
                return 1;
            }

            const std::chrono::duration<double, std::milli> elapsed_ms = end - start;
            durations_ms.push_back(elapsed_ms.count());
        }

        const auto match_stats = bench::compute_stats(durations_ms);
        bench::print_csv_row(
            "css_selector_bench",
            "match",
            selector_text,
            html.size(),
            match_iterations,
            match_count,
            match_stats,
            bench::throughput_mib_s(html.size(), match_stats.avg_ms));
    }

    return 0;
}
