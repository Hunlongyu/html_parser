#include "html5lib_tree_support.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#ifndef HPS_HTML5LIB_TREE_DIR
#error "HPS_HTML5LIB_TREE_DIR must be defined (path to upstream html5lib tree-construction .dat files)"
#endif

// 完整上游 html5lib tree-construction 一致性套件。
// 这是“报告型”测试：跑完整套件并打印通过率（与 parse5/lexbor 同口径），
// 但**不因个案不符而失败**——本库的 tree builder 目前刻意比完整 HTML5 轻量，
// 失败属预期。它只在“套件无法加载/全军覆没”时失败，便于在 CI 里跟踪通过率走向。
TEST(HTML5LibTreeConstructionConformance, FullUpstreamSuitePassRate) {
    namespace ts = hps::test_support;

    const std::filesystem::path tree_dir{HPS_HTML5LIB_TREE_DIR};
    ASSERT_TRUE(std::filesystem::is_directory(tree_dir)) << tree_dir;

    const auto cases = ts::collect_cases_from_dir(tree_dir);
    ASSERT_FALSE(cases.empty()) << "no .dat cases found under " << tree_dir;

    hps::Options options;
    options.comment_mode    = hps::CommentMode::Preserve;
    options.whitespace_mode = hps::WhitespaceMode::Preserve;
    options.decode_entities = true;  // html5lib 期望树按规范解码字符引用

    size_t total = 0;
    size_t passed = 0;
    size_t errored = 0;  // 解析抛异常的个案（计入失败）
    std::vector<std::string> sample_failures;

    for (const auto& test_case : cases) {
        if (!test_case.scripting_enabled) {
            continue;  // script-off 变体需要脚本状态语义，跳过
        }
        ++total;

        hps::ParseResult result;
        try {
            if (test_case.fragment_context.empty()) {
                result = hps::parse_with_error(test_case.input, options);
            } else {
                result = hps::parse_fragment_with_error(
                    test_case.input, ts::first_non_empty_line(test_case.fragment_context), options);
            }
        } catch (const std::exception&) {
            ++errored;
            continue;
        }

        if (result.document == nullptr) {
            continue;
        }

        const std::string actual = ts::serialize_document_tree(*result.document);
        if (actual == test_case.expected_document) {
            ++passed;
        } else if (sample_failures.size() < 20) {
            sample_failures.push_back(test_case.description);
        }
    }

    const double rate = total > 0 ? (100.0 * static_cast<double>(passed) / static_cast<double>(total)) : 0.0;

    std::cout << "\n==================== html5lib tree-construction 一致性 ====================\n"
              << "  passed " << passed << " / " << total << "  ("
              << std::fixed << std::setprecision(2) << rate << "%)";
    if (errored > 0) {
        std::cout << "   [解析抛异常: " << errored << "]";
    }
    std::cout << "\n  失败样例(前 " << sample_failures.size() << "):\n";
    for (const auto& description : sample_failures) {
        std::cout << "    - " << description << '\n';
    }
    std::cout << "=========================================================================\n";

    // 报告型断言：打印通过率，同时设一个回退地板，防止大幅退化被悄悄合入。
    // 地板取当前通过数下方的整数（当前 1227），上游套件小幅增删不会误伤。
    RecordProperty("total", static_cast<int>(total));
    RecordProperty("passed", static_cast<int>(passed));
    static constexpr size_t kConformanceFloor = 1200;
    EXPECT_GT(total, 0u);
    EXPECT_GE(passed, kConformanceFloor)
        << "html5lib tree-construction 通过数跌破地板 " << kConformanceFloor << "，疑似一致性回退";
}
