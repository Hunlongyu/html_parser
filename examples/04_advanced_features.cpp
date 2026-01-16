/**
 * @file 04_advanced_features.cpp
 * @brief 高级功能示例：错误处理、文件解析与选项配置 / Advanced Features Example: Error Handling, File Parsing, and Options
 *
 * 本示例展示了 HTML Parser 的高级功能：
 * 1. 错误处理与容错机制 (Strict vs Lenient 模式)
 * 2. 解析文件而非字符串
 * 3. 自定义解析选项 (Options)
 *
 * This example demonstrates advanced features of HTML Parser:
 * 1. Error handling and fault tolerance (Strict vs Lenient mode)
 * 2. Parsing files instead of strings
 * 3. Custom parsing options
 */

#include "hps/hps.hpp"

#include <fstream>
#include <iostream>

// 创建一个临时的 HTML 文件用于测试
// Create a temporary HTML file for testing
static void create_temp_file(const std::string& filename, const std::string& content) {
    std::ofstream out(filename);
    out << content;
    out.close();
}

int main() {
    // 1. 错误处理示例
    // 1. Error Handling Example
    std::string broken_html = "<html><body><div>Unclosed Div</body></html>";

    std::cout << ">>> Parsing broken HTML (Default/Lenient Mode)" << std::endl;
    // 默认模式 (Lenient) 会尝试修复错误
    // Default mode (Lenient) will try to fix errors
    auto doc_lenient = hps::parse(broken_html);
    if (doc_lenient) {
        std::cout << "Parsed successfully! (Lenient)" << std::endl;
    }

    std::cout << "\n>>> Parsing broken HTML (Strict Mode)" << std::endl;
    // 严格模式 (Strict) 会报告错误
    // Strict mode will report errors
    // 注意：hps::parse 在 strict 模式下遇到错误会抛出异常或者返回空？
    // 根据 API，hps::parse_with_error 返回 ParseResult，包含 errors
    // According to API, hps::parse_with_error returns ParseResult containing errors

    hps::Options strict_opts = hps::Options::strict();

    // 使用 parse_with_error 获取详细错误信息
    // Use parse_with_error to get detailed error info
    auto result = hps::parse_with_error(broken_html, strict_opts);

    if (result.has_errors()) {
        std::cout << "Parsing failed as expected! Found " << result.error_count() << " errors:" << std::endl;
        for (const auto& err : result.errors) {
            std::cout << "Error: " << err.message << " at line " << err.location.line << ", column " << err.location.column << std::endl;
        }
    } else {
        std::cout << "Unexpected success in strict mode!" << std::endl;
    }

    // 2. 自定义选项示例
    // 2. Custom Options Example
    std::cout << "\n>>> Custom Options: Removing Comments" << std::endl;

    std::string html_with_comments = R"(
        <div>
            <!-- This is a secret comment -->
            <p>Visible content</p>
        </div>
    )";

    hps::Options perf_opts = hps::Options::performance();
    // 性能模式默认移除注释
    // Performance mode removes comments by default

    auto doc_perf = hps::parse(html_with_comments, perf_opts);
    if (doc_perf) {
        // 检查注释是否存在 (简单检查文本内容)
        // Check if comment exists (simple text check)
        std::string text = doc_perf->text_content();
        std::cout << "Parsed Content: " << text << std::endl;
        // 应该只看到 "Visible content"
        // Should only see "Visible content"
    }

    // 3. 文件解析示例
    // 3. File Parsing Example
    std::string filename = "test_example.html";
    create_temp_file(filename, "<html><head><title>File Test</title></head><body><h1>From File</h1></body></html>");

    std::cout << "\n>>> Parsing from file: " << filename << std::endl;
    auto doc_file = hps::parse_file(filename);

    if (doc_file) {
        std::cout << "File parsed successfully!" << std::endl;
        std::cout << "Title: " << doc_file->title() << std::endl;
    } else {
        std::cout << "Failed to parse file!" << std::endl;
    }

    // 清理临时文件
    // Clean up temp file
    std::remove(filename.c_str());

    system("pause");
    return 0;
}
