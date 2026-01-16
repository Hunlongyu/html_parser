#include "hps/core/element.hpp"
#include "hps/parsing/html_parser.hpp"
#include "hps/query/css/css_parser.hpp"
#include "hps/query/css/css_selector.hpp"
#include "hps/query/css/css_matcher.hpp"

#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>
#include <string>
#include <memory>

using namespace hps;

// 生成测试用的 HTML
static std::string generate_benchmark_html(const size_t repeat_count) {
    std::string html = "<!DOCTYPE html><html><head><title>CSS Benchmark</title></head><body>";
    std::string content = R"(
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

    for (size_t i = 0; i < repeat_count; ++i) {
        html += "<div id='wrapper-" + std::to_string(i) + "'>";
        html += content;
        html += "</div>";
    }

    html += "</body></html>";
    return html;
}

int main() {
    // 1. 准备数据
    constexpr size_t REPEAT_COUNT = 1000;
    std::cout << "Generating HTML data..." << std::endl;
    std::string html = generate_benchmark_html(REPEAT_COUNT);
    std::cout << "HTML size: " << html.length() / 1024 << " KB" << std::endl;

    std::cout << "Parsing HTML..." << std::endl;
    HTMLParser html_parser;
    auto document = html_parser.parse(html);
    if (!document) {
        std::cerr << "Failed to parse HTML" << std::endl;
        return 1;
    }
    std::cout << "HTML parsed." << std::endl;

    // 测试选择器集合
    std::vector<std::string> selectors = {
        // 基础选择器
        "div", 
        ".container", 
        "#main-content",
        // 属性选择器
        "[type='text']",
        "a[href='#']",
        // 组合器
        "div > p",
        "ul li a",
        "h1 + nav",
        // 伪类
        "li:first-child",
        "li:nth-child(2)",
        "input:checked",
        "button:disabled",
        // 复杂选择器
        "div.container > header nav ul li a.active",
        "section.section h2 + p.description"
    };

    constexpr int ITERATIONS = 50;
    
    // 2. Benchmark: 解析选择器 (Parsing)
    {
        std::cout << "\n=== Benchmark: CSS Selector Parsing (" << ITERATIONS << " iterations) ===" << std::endl;
        std::vector<double> durations;
        
        for (int i = 0; i < ITERATIONS; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            
            for (const auto& sel_str : selectors) {
                // 每次都重新创建 parser 以测试解析性能
                CSSParser parser(sel_str);
                auto selector = parser.parse_selector_list();
                if (!selector) {
                    std::cerr << "Failed to parse: " << sel_str << std::endl;
                }
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> ms = end - start;
            durations.push_back(ms.count());
        }
        
        double total = std::accumulate(durations.begin(), durations.end(), 0.0);
        double avg = total / ITERATIONS;
        std::cout << "Total Parsing Time (avg): " << avg << " ms" << std::endl;
        std::cout << "Per Selector (avg): " << (avg / selectors.size()) * 1000.0 << " us" << std::endl;
    }

    // 3. Benchmark: 匹配选择器 (Matching)
    {
        std::cout << "\n=== Benchmark: CSS Selector Matching (" << ITERATIONS << " iterations) ===" << std::endl;
        
        // 预先解析所有选择器
        std::vector<std::unique_ptr<SelectorList>> parsed_selectors;
        for (const auto& sel_str : selectors) {
            CSSParser parser(sel_str);
            parsed_selectors.push_back(parser.parse_selector_list());
        }
        
        std::vector<double> durations;
        size_t total_matches = 0;
        
        for (int i = 0; i < ITERATIONS; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            size_t current_matches = 0;
            
            for (const auto& selector : parsed_selectors) {
                if (selector) {
                    auto results = CSSMatcher::find_all(*document, *selector);
                    current_matches += results.size();
                }
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> ms = end - start;
            durations.push_back(ms.count());
            
            if (i == 0) total_matches = current_matches;
        }
        
        double total = std::accumulate(durations.begin(), durations.end(), 0.0);
        double avg = total / ITERATIONS;
        std::cout << "Total Matching Time (avg): " << avg << " ms" << std::endl;
        std::cout << "Total Elements Matched: " << total_matches << std::endl;
    }

    return 0;
}
