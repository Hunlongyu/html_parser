#include "hps/parsing/tree_builder.hpp"

#include "hps/core/document.hpp"
#include "hps/core/text_node.hpp"
#include "hps/parsing/tokenizer.hpp"

#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

static std::string read_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开文件: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void print_node(const hps::Node* node, int depth = 0) {
    if (!node)
        return;

    // 打印缩进
    for (int i = 0; i < depth; ++i) {
        std::cout << "  ";
    }

    // 根据节点类型打印信息
    switch (node->type()) {
        case hps::NodeType::Element: {
            const auto element = node->as_element();
            std::cout << "<" << element->tag_name();

            // 打印属性
            for (const auto& attr : element->attributes()) {
                std::cout << " " << attr.name() << "=\"" << attr.value() << "\"";
            }
            std::cout << ">" << std::endl;

            // 递归打印子节点
            for (const auto& child : element->children()) {
                print_node(child.get(), depth + 1);
            }

            // 打印结束标签
            for (int i = 0; i < depth; ++i) {
                std::cout << "  ";
            }
            std::cout << "</" << element->tag_name() << ">" << std::endl;
            break;
        }
        case hps::NodeType::Text: {
            const auto  text_node = node->as_text();
            std::string text      = text_node->normalized_text();
            if (!text.empty()) {
                std::cout << "TEXT: \"" << text << "\"" << std::endl;
            }
            break;
        }
        case hps::NodeType::Document:
            std::cout << "DOCUMENT" << std::endl;
            for (const auto& child : node->children()) {
                print_node(child.get(), depth + 1);
            }
            break;
        default:
            std::cout << "OTHER NODE TYPE" << std::endl;
            break;
    }
}

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    std::string html = read_file("./html/complex.html");

    try {
        // 创建文档对象
        auto document = std::make_shared<hps::Document>(html);

        // 创建TreeBuilder
        hps::TreeBuilder builder(document);

        // 创建Tokenizer
        hps::Tokenizer tokenizer(html);

        std::cout << "开始解析..." << std::endl;

        // 解析过程
        while (true) {
            auto token = tokenizer.next_token();
            if (!token)
                break;

            // 显示Token信息（可选）
            std::cout << "处理Token: ";
            switch (token->type()) {
                case hps::TokenType::OPEN:
                    std::cout << "OPEN <" << token->name() << ">";
                    break;
                case hps::TokenType::CLOSE:
                    std::cout << "CLOSE </" << token->name() << ">";
                    break;
                case hps::TokenType::CLOSE_SELF:
                    std::cout << "SELF_CLOSE <" << token->name() << " />";
                    break;
                case hps::TokenType::TEXT:
                    std::cout << "TEXT: \"" << token->value() << "\"";
                    break;
                case hps::TokenType::COMMENT:
                    std::cout << "COMMENT: \"" << token->value() << "\"";
                    break;
                case hps::TokenType::DONE:
                    std::cout << "DONE";
                    break;
                default:
                    std::cout << "OTHER";
                    break;
            }
            std::cout << std::endl;

            // 处理Token
            if (!builder.process_token(*token)) {
                std::cerr << "处理Token失败!" << std::endl;
                break;
            }

            if (token->type() == hps::TokenType::DONE) {
                break;
            }
        }

        // 完成构建
        if (!builder.finish()) {
            std::cerr << "构建文档失败!" << std::endl;
        }

        std::cout << "\n解析完成!" << std::endl;

        // 检查错误
        const auto& errors = builder.errors();
        if (!errors.empty()) {
            std::cout << "\n解析错误:" << std::endl;
            for (const auto& error : errors) {
                std::cout << "- " << error.message << std::endl;
            }
        }

        std::cout << "\n=== DOM树结构 ===" << std::endl;
        print_node(document.get());

        std::cout << "\n=== 元素查询演示 ===" << std::endl;

        // 遍历所有子节点寻找元素
        std::function<void(const hps::Node*, const std::string&)> find_elements;
        find_elements = [&](const hps::Node* node, const std::string& tag_name) {
            if (node->type() == hps::NodeType::Element) {
                const auto element = node->as_element();
                if (element->tag_name() == tag_name) {
                    std::cout << "找到 <" << tag_name << "> 元素";
                    if (element->has_attribute("class")) {
                        std::cout << " class=\"" << element->get_attribute("class") << "\"";
                    }
                    if (element->has_attribute("id")) {
                        std::cout << " id=\"" << element->get_attribute("id") << "\"";
                    }
                    std::cout << std::endl;
                }
            }

            // 递归查找子节点
            for (const auto& child : node->children()) {
                find_elements(child.get(), tag_name);
            }
        };

        // 查找特定元素
        std::cout << "查找所有 div 元素:" << std::endl;
        find_elements(document.get(), "div");

        std::cout << "\n查找所有 span 元素:" << std::endl;
        find_elements(document.get(), "span");

        std::cout << "\n=== 文本内容提取 ===" << std::endl;

        // 提取所有文本内容
        std::function<void(const hps::Node*)> extract_text;
        extract_text = [&](const hps::Node* node) {
            if (node->type() == hps::NodeType::Text) {
                const auto text_node = node->as_text();
                std::string text      = std::string(text_node->text());
                text.erase(0, text.find_first_not_of(" \t\n\r"));
                text.erase(text.find_last_not_of(" \t\n\r") + 1);
                if (!text.empty()) {
                    std::cout << "文本: \"" << text << "\"" << std::endl;
                }
            }

            for (const auto& child : node->children()) {
                extract_text(child.get());
            }
        };

        extract_text(document.get());

    } catch (const std::exception& e) {
        std::cerr << "解析异常: " << e.what() << std::endl;
    }

    system("pause");
    return 0;
}