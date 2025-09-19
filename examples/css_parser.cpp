#include "hps/query/css/css_parser.hpp"

#include "hps/parsing/options.hpp"
#include "hps/query/css/css_utils.hpp"
#include "hps/utils/exception.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace hps;

// 辅助函数：打印选择器信息
void print_selector_info(const std::unique_ptr<CSSSelector>& selector, int indent = 0) {
    if (!selector) {
        std::cout << std::string(indent, ' ') << "null selector" << std::endl;
        return;
    }

    std::cout << std::string(indent, ' ') << "Selector: " << selector->to_string() << " (Type: " << static_cast<int>(selector->type()) << ")" << std::endl;

    // 显示选择器特异性
    auto specificity = selector->calculate_specificity();
    std::cout << std::string(indent, ' ') << "Specificity: (" << specificity.inline_style << ", " << specificity.ids << ", " << specificity.classes << ", " << specificity.elements << ")" << std::endl;
}

// 辅助函数：打印选择器列表
void print_selector_list(const std::unique_ptr<SelectorList>& list) {
    if (!list) {
        std::cout << "Empty selector list" << std::endl;
        return;
    }

    std::cout << "Selector List (" << list->size() << " selectors):" << std::endl;
    const auto& selectors = list->selectors();
    for (size_t i = 0; i < selectors.size(); ++i) {
        std::cout << "  [" << i << "] ";
        print_selector_info(selectors[i], 4);
    }
}

// 辅助函数：打印解析错误
void print_errors(const std::vector<HPSError>& errors) {
    if (errors.empty()) {
        std::cout << "No parsing errors." << std::endl;
        return;
    }

    std::cout << "Parsing errors (" << errors.size() << "):" << std::endl;
    for (const auto& error : errors) {
        std::cout << "  - Line " << error.location.line << ", Column " << error.location.column << ": " << error.message << std::endl;
    }
}

// 测试词法分析器
void test_lexer() {
    std::cout << "\n=== CSS Lexer Tests ===" << std::endl;

    std::vector<std::string> test_inputs = {"div", ".class-name", "#my-id", "[attr=value]", "div.class#id", "div > p + span", "input[type=\"text\"]", "p:first-child", "::before", "div ~ p", ":not(.hidden)", ":nth-child(2n+1)", "input:disabled", "a:hover::after"};

    for (const auto& input : test_inputs) {
        std::cout << "\nTokenizing: \"" << input << "\"" << std::endl;

        try {
            CSSLexer lexer(input);

            while (lexer.has_more_tokens()) {
                auto token = lexer.next_token();
                std::cout << "  Token: " << token.value << " (Type: " << static_cast<int>(token.type) << ", Pos: " << token.position << ")" << std::endl;

                if (token.type == CSSLexer::CSSTokenType::EndOfFile) {
                    break;
                }
            }
        } catch (const HPSException& e) {
            std::cout << "  Error: " << e.what() << std::endl;
        }
    }
}

// 测试解析器
void test_parser() {
    std::cout << "\n=== CSS Parser Tests ===" << std::endl;

    std::vector<std::string> test_selectors = {// 基本选择器
                                               "div",
                                               ".container",
                                               "#header",
                                               "*",

                                               // 属性选择器
                                               "[data-id]",
                                               "[type=\"text\"]",
                                               "[class*=\"btn\"]",
                                               "[href^=\"https\"]",
                                               "[src$=\".jpg\"]",
                                               "[lang|=\"en\"]",
                                               "[title~=\"important\"]",

                                               // 组合选择器
                                               "div p",
                                               "ul > li",
                                               "h1 + p",
                                               "h2 ~ p",

                                               // 复合选择器
                                               "div.container#main",
                                               "input[type=\"text\"].form-control",
                                               "button.btn.btn-primary:hover",

                                               // 伪类选择器（新增功能）
                                               "li:first-child",
                                               "tr:last-child",
                                               "div:nth-child(2n+1)",
                                               "p:nth-last-child(3)",
                                               "span:first-of-type",
                                               "img:last-of-type",
                                               "section:only-child",
                                               "article:only-of-type",
                                               "div:empty",
                                               "html:root",
                                               "input:not([disabled])",
                                               "a:hover",
                                               "button:active",
                                               "input:focus",
                                               "a:visited",
                                               "a:link",
                                               "input:disabled",
                                               "button:enabled",
                                               "checkbox:checked",

                                               // 伪元素选择器
                                               "p::first-line",
                                               "p::first-letter",
                                               "div::before",
                                               "span::after",

                                               // 兄弟选择器（新增功能）
                                               "h1 + p",
                                               "h2 ~ p",
                                               "div.header + div.content",
                                               "li:first-child ~ li",

                                               // 选择器列表
                                               "h1, h2, h3",
                                               "div.content, section.main, article",
                                               "input:focus, button:hover, a:active",

                                               // 复杂选择器
                                               "nav ul li a:hover",
                                               "form input[type=\"submit\"]:not(:disabled)",
                                               "article > header h1:first-child",
                                               "div.container > section.main + aside.sidebar",
                                               "table tr:nth-child(even) td:first-child",

                                               // 包含中文的测试用例（测试字符安全处理）
                                               "div[title=\"测试\"]",
                                               ".中文类名",
                                               "#中文ID",

                                               // 错误的选择器（用于测试错误处理）
                                               "div[",
                                               ".class..invalid",
                                               "#id#invalid",
                                               "::",
                                               ":::",
                                               "[attr=]"};

    // 测试不同的解析选项
    std::vector<std::pair<std::string, Options>> option_tests = {{"Lenient Mode", Options{}},  // 默认宽松模式
                                                                 {"Strict Mode", []() {
                                                                      Options opts;
                                                                      opts.error_handling = ErrorHandlingMode::Strict;
                                                                      return opts;
                                                                  }()}};

    for (const auto& [mode_name, options] : option_tests) {
        std::cout << "\n--- " << mode_name << " ---" << std::endl;

        for (const auto& selector : test_selectors) {
            std::cout << "\nParsing: \"" << selector << "\"" << std::endl;

            try {
                CSSParser parser(selector, options);
                auto      result = parser.parse_selector_list();

                if (result) {
                    print_selector_list(result);
                } else {
                    std::cout << "  Failed to parse selector" << std::endl;
                }

                // 打印解析错误
                print_errors(parser.get_errors());

            } catch (const HPSException& e) {
                std::cout << "  Exception: " << e.what() << " (Code: " << static_cast<int>(e.code()) << ")" << std::endl;
            }
        }
    }
}

// 测试工具函数
void test_utility_functions() {
    std::cout << "\n=== Utility Functions Tests ===" << std::endl;

    std::vector<std::string> test_selectors = {
        "div.container", "  h1   ,   h2  ,  h3  ", "input[type=\"text\"]", "invalid[[", "p::first-line", "DIV.CLASS#ID", "div:first-child + p:last-child", "section ~ article:not(.hidden)", "input:disabled::placeholder", "div[data-测试=\"值\"]"};

    for (const auto& selector : test_selectors) {
        std::cout << "\nTesting: \"" << selector << "\"" << std::endl;

        // 测试选择器验证
        bool is_valid = is_valid_selector(selector);
        std::cout << "  Valid: " << (is_valid ? "Yes" : "No") << std::endl;

        // 测试选择器规范化
        if (is_valid) {
            std::string normalized = normalize_selector(selector);
            std::cout << "  Normalized: \"" << normalized << "\"" << std::endl;

            // 测试便利解析函数
            try {
                auto result = parse_css_selector(selector);
                std::cout << "  Parsed successfully (" << result->size() << " selectors)" << std::endl;

                // 显示解析结果的详细信息
                for (size_t i = 0; i < result->size(); ++i) {
                    const auto& sel  = result->selectors()[i];
                    auto        spec = sel->calculate_specificity();
                    std::cout << "    [" << i << "] " << sel->to_string() << " - Specificity: (" << spec.inline_style << "," << spec.ids << "," << spec.classes << "," << spec.elements << ")" << std::endl;
                }
            } catch (const HPSException& e) {
                std::cout << "  Parse error: " << e.what() << std::endl;
            }
        }
    }
}

// 测试批量解析
void test_batch_parsing() {
    std::cout << "\n=== Batch Parsing Tests ===" << std::endl;

    std::vector<std::string_view> selectors = {"div", ".container", "#header", "nav ul li a", "input[type=\"text\"]:focus", "p::first-line", "div:first-child + p:last-child", "section ~ article:not(.hidden)", "table tr:nth-child(even) td", "form input:disabled::placeholder"};

    try {
        auto results = parse_css_selectors(selectors);

        std::cout << "Batch parsed " << results.size() << " selectors:" << std::endl;
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << "  [" << i << "] \"" << selectors[i] << "\" -> ";
            if (results[i]) {
                std::cout << results[i]->size() << " parsed selectors" << std::endl;
            } else {
                std::cout << "Failed to parse" << std::endl;
            }
        }
    } catch (const HPSException& e) {
        std::cout << "Batch parsing error: " << e.what() << std::endl;
    }
}

// 性能测试
void test_performance() {
    std::cout << "\n=== Performance Tests ===" << std::endl;

    const std::vector<std::string> complex_selectors = {"nav.main-nav ul.menu li.item:nth-child(odd) a[href^=\"https\"]:hover::before",
                                                        "div.container > section.content + aside.sidebar ~ footer.main-footer",
                                                        "table.data tr:nth-child(even) td:first-child input[type=\"checkbox\"]:checked",
                                                        "form.search input[type=\"text\"]:not(:disabled):focus + button.submit:hover",
                                                        "article.post > header.post-header h1.title:first-child::first-letter"};

    const int iterations = 1000;

    for (const auto& selector : complex_selectors) {
        std::cout << "\nTesting selector: \"" << selector << "\"" << std::endl;
        std::cout << "Parsing " << iterations << " times..." << std::endl;

        auto start             = std::chrono::high_resolution_clock::now();
        int  successful_parses = 0;

        for (int i = 0; i < iterations; ++i) {
            try {
                auto result = parse_css_selector(selector);
                if (result && !result->empty()) {
                    successful_parses++;
                }
            } catch (const HPSException& e) {
                std::cout << "Error at iteration " << i << ": " << e.what() << std::endl;
                break;
            }
        }

        auto end      = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "Successful parses: " << successful_parses << "/" << iterations << std::endl;
        std::cout << "Total time: " << duration.count() << " microseconds" << std::endl;
        if (successful_parses > 0) {
            std::cout << "Average time per parse: " << (duration.count() / successful_parses) << " microseconds" << std::endl;
        }
    }
}

// 测试字符安全处理
void test_character_safety() {
    std::cout << "\n=== Character Safety Tests ===" << std::endl;

    std::vector<std::string> unicode_selectors = {"div[title=\"测试内容\"]", ".中文类名", "#中文标识符", "p[data-名称=\"值\"]", "span:not([class*=\"测试\"])", "div.容器 > p.内容", "input[placeholder=\"请输入内容\"]:focus"};

    std::cout << "Testing selectors with Unicode characters:" << std::endl;

    for (const auto& selector : unicode_selectors) {
        std::cout << "\nTesting: \"" << selector << "\"" << std::endl;

        try {
            auto result = parse_css_selector(selector);
            if (result) {
                std::cout << "  ✓ Parsed successfully (" << result->size() << " selectors)" << std::endl;
                for (size_t i = 0; i < result->size(); ++i) {
                    std::cout << "    [" << i << "] " << result->selectors()[i]->to_string() << std::endl;
                }
            } else {
                std::cout << "  ✗ Failed to parse" << std::endl;
            }
        } catch (const HPSException& e) {
            std::cout << "  ✗ Exception: " << e.what() << std::endl;
        }
    }
}

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
    std::cout << "CSS Parser Example and Test Suite" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Version: Enhanced with new selector support" << std::endl;
    std::cout << "Features: Pseudo-classes, Sibling selectors, Unicode support" << std::endl;

    try {
        // 运行各种测试
        test_lexer();
        test_parser();
        test_utility_functions();
        test_batch_parsing();
        test_performance();
        test_character_safety();

        std::cout << "\n=== All Tests Completed Successfully ===" << std::endl;
        std::cout << "\nKey improvements in this version:" << std::endl;
        std::cout << "• Enhanced pseudo-class selector support" << std::endl;
        std::cout << "• Sibling selector functionality (+ and ~)" << std::endl;
        std::cout << "• Safe Unicode character handling" << std::endl;
        std::cout << "• Improved error reporting and debugging" << std::endl;
        std::cout << "• Comprehensive selector specificity calculation" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\nUnexpected error: " << e.what() << std::endl;
        return 1;
    }

#ifdef _WIN32
    system("pause");
#endif
    return 0;
}