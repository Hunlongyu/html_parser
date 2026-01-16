#include "hps/parsing/tokenizer.hpp"

#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>

using namespace hps;

// 用于生成大段 HTML 内容的辅助函数
static std::string generate_large_html(const size_t repeat_count) {
    std::string html    = "<!DOCTYPE html><html><head><title>Benchmark</title></head><body>";
    std::string content = R"(
        <div class="container" id="main" data-test="value">
            <h1>Performance Test</h1>
            <p class="text">Lorem ipsum dolor sit amet, consectetur adipiscing elit. 
            Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. 
            Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.</p>
            <ul>
                <li>Item 1</li>
                <li>Item 2</li>
                <li>Item 3</li>
            </ul>
            <img src="image.jpg" alt="test image" width="100" height="100" />
            <!-- This is a comment -->
        </div>
    )";

    for (size_t i = 0; i < repeat_count; ++i) {
        html += content;
    }

    html += "</body></html>";
    return html;
}

int main() {
    // 1. 准备测试数据
    constexpr size_t REPEAT_COUNT = 10000;  // 重复次数，生成足够大的文件
    std::cout << "Generating test data..." << '\n';
    std::string source       = generate_large_html(REPEAT_COUNT);
    size_t      data_size_mb = source.length() / (1024 * 1024);
    std::cout << "Data size: " << source.length() << " bytes (~" << data_size_mb << " MB)" << '\n';

    // 2. 预热
    {
        Tokenizer tokenizer(source, Options::performance());
        tokenizer.tokenize_all();
    }

    // 3. 执行测试
    constexpr int       ITERATIONS = 10;
    std::vector<double> durations;
    size_t              total_tokens = 0;

    std::cout << "Running benchmark (" << ITERATIONS << " iterations)..." << '\n';

    for (int i = 0; i < ITERATIONS; ++i) {
        auto start = std::chrono::high_resolution_clock::now();

        Tokenizer tokenizer(source, Options::performance());
        auto      tokens = tokenizer.tokenize_all();

        auto                                      end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms  = end - start;
        durations.push_back(ms.count());

        if (i == 0)
            total_tokens = tokens.size();
    }

    // 4. 计算统计结果
    double total_time = std::accumulate(durations.begin(), durations.end(), 0.0);
    double avg_time   = total_time / ITERATIONS;
    double min_time   = *std::ranges::min_element(durations);
    double max_time   = *std::ranges::max_element(durations);

    // 计算吞吐量 (MB/s)
    double throughput = (static_cast<double>(source.length()) / (1024.0 * 1024.0)) / (avg_time / 1000.0);

    std::cout << "\n=== Benchmark Results ===" << '\n';
    std::cout << "Total Tokens: " << total_tokens << '\n';
    std::cout << "Average Time: " << avg_time << " ms" << '\n';
    std::cout << "Min Time:     " << min_time << " ms" << '\n';
    std::cout << "Max Time:     " << max_time << " ms" << '\n';
    std::cout << "Throughput:   " << throughput << " MB/s" << '\n';

    return 0;
}
