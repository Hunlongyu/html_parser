/**
 * @file 01_basic_parsing.cpp
 * @brief 基础HTML解析示例 / Basic HTML Parsing Example
 *
 * 本示例展示了如何使用 HTML Parser 进行最基础的 HTML 解析操作：
 * 1. 解析简单的 HTML 字符串
 * 2. 访问文档根节点
 * 3. 递归遍历 DOM 树并打印节点信息
 *
 * This example demonstrates basic HTML parsing operations using HTML Parser:
 * 1. Parsing a simple HTML string
 * 2. Accessing the document root node
 * 3. Recursively traversing the DOM tree and printing node information
 */

#include "hps/core/element.hpp"
#include "hps/core/text_node.hpp"
#include "hps/hps.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

/**
 * @brief 辅助函数：打印缩进
 * @brief Helper function: Print indentation
 */
void print_indent(int depth) {
    for (int i = 0; i < depth; ++i) {
        std::cout << "  ";
    }
}

/**
 * @brief 递归打印 DOM 树
 * @brief Recursively print the DOM tree
 *
 * @param node 当前节点 / Current node
 * @param depth 当前深度 / Current depth
 */
void print_tree(const hps::Node* node, int depth = 0) {
    if (!node)
        return;

    print_indent(depth);

    // 根据节点类型打印不同信息 / Print information based on node type
    switch (node->type()) {
        case hps::NodeType::Document:
            std::cout << "[Document]" << std::endl;
            break;
        case hps::NodeType::Element: {
            const auto* element = node->as_element();
            std::cout << "[Element] <" << element->tag_name() << ">";

            // 打印属性数量 / Print attribute count
            if (element->attribute_count() > 0) {
                for (const auto& attr : element->attributes()) {
                    std::cout << " " << attr.name();
                    if (attr.has_value()) {
                        std::cout << "=\"" << attr.value() << "\"";
                    }
                }
            }
            std::cout << std::endl;
            break;
        }
        case hps::NodeType::Text: {
            const auto* text = node->as_text();
            // 简单的文本清理，去除多余空白以便打印 / Simple text cleaning for printing
            std::string content = text->text_content();
            if (content.length() > 20) {
                content = content.substr(0, 20) + "...";
            }
            // 替换换行符 / Replace newlines
            size_t pos = 0;
            while ((pos = content.find('\n', pos)) != std::string::npos) {
                content.replace(pos, 1, "\\n");
                pos += 2;
            }
            std::cout << "[Text] \"" << content << "\"" << std::endl;
            break;
        }
        case hps::NodeType::Comment: {
            const auto* comment = node->as_comment();
            std::cout << "[Comment] " << comment->text_content() << std::endl;
            break;
        }
        default:
            std::cout << "[Unknown]" << std::endl;
            break;
    }

    // 递归遍历子节点 / Recursively traverse children
    for (const auto* child : node->children()) {
        print_tree(child, depth + 1);
    }
}

int main() {
    // 1. 定义简单的 HTML 字符串
    // 1. Define a simple HTML string
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Basic Example</title>
        </head>
        <body>
            <h1>Hello, HTML Parser!</h1>
            <p>This is a <b>simple</b> example.</p>
            <!-- This is a comment -->
            <div id="content">
                <ul>
                    <li>Item 1</li>
                    <li>Item 2</li>
                </ul>
            </div>
        </body>
        </html>
    )";

    std::cout << "Parsing HTML string..." << std::endl;

    // 2. 解析 HTML
    // 2. Parse HTML
    // hps::parse 返回一个 std::shared_ptr<hps::Document>
    // hps::parse returns a std::shared_ptr<hps::Document>
    auto doc = hps::parse(html);

    if (!doc) {
        std::cerr << "Failed to parse HTML" << std::endl;
        return 1;
    }

    std::cout << "Parsing successful! Traversing DOM tree:" << std::endl;

    // 3. 遍历并打印 DOM 树
    // 3. Traverse and print DOM tree
    print_tree(doc.get());

    system("pause");
    return 0;
}
