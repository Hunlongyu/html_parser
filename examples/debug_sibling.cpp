#include "hps/query/css/css_matcher.hpp"
#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/parsing/html_parser.hpp"
#include "hps/query/css/css_parser.hpp"

#include <iostream>
#include <memory>

using namespace hps;

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    try {
        HTMLParser parser;
        const std::string html = R"(
            <div>
                <h1>标题</h1>
                <p class="intro">介绍段落</p>
                <p class="content">内容段落1</p>
            </div>
        )";

        auto document = parser.parse(html);
        auto root = document->root();

        // 调试：打印DOM结构
        std::cout << "=== DOM结构 ===" << std::endl;
        auto container = CSSMatcher::find_first(*document, *parse_css_selector("div"));
        if (container) {
            for (const auto& child : container->children()) {
                if (child->is_element()) {
                    auto elem = child->as_element();
                    std::cout << "<" << elem->tag_name() << ">";
                    if (elem->has_attribute("class")) {
                        std::cout << " class=\"" << elem->get_attribute("class") << "\"";
                    }
                    std::cout << " - 内容: \"" << elem->text_content() << "\"" << std::endl;
                }
            }
        }

        // 测试相邻兄弟选择器
        std::cout << "\n=== 相邻兄弟选择器测试 ===" << std::endl;
        auto selector_list = parse_css_selector("h1 + p");
        std::cout << "解析的选择器: " << selector_list->to_string() << std::endl;
        
        auto results = CSSMatcher::find_all(*document, *selector_list);
        std::cout << "找到 " << results.size() << " 个匹配:" << std::endl;
        for (const auto& element : results) {
            std::cout << "  - <" << element->tag_name() << ">";
            if (element->has_attribute("class")) {
                std::cout << " class=\"" << element->get_attribute("class") << "\"";
            }
            std::cout << " - 内容: \"" << element->text_content() << "\"" << std::endl;
        }

        // 手动测试每个p元素
        std::cout << "\n=== 手动测试每个p元素 ===" << std::endl;
        auto all_p = CSSMatcher::find_all(*document, *parse_css_selector("p"));
        for (const auto& p : all_p) {
            const bool matches = selector_list->matches(*p);
            std::cout << "<" << p->tag_name() << ">";
            if (p->has_attribute("class")) {
                std::cout << " class=\"" << p->get_attribute("class") << "\"";
            }
            std::cout << " 匹配 'h1 + p': " << (matches ? "是" : "否") << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

#ifdef _WIN32
    system("pause");
#endif
    return 0;
}