#include "hps/hps.hpp"
#include "hps/query/css/css_parser.hpp"
#include "hps/query/css/css_utils.hpp"

#include <iostream>

using namespace hps;

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    try {
        std::string html = R"(
        <div class="entry-content">
            <p>
                <span>这是 span</span>
                <a href="#">这是链接</a>
                文本内容
            </p>
        </div>
    )";

        // 解析HTML
        auto res = parse(html);

        // 解析 CSS 选择器 ".entry-content p > :not(span)"
        std::cout << "=== 测试CSS选择器解析 ===" << std::endl;
        CSSParser parser(".entry-content p > :not(span)");
        auto      selector_list = parser.parse_selector_list();

        if (selector_list) {
            std::cout << "解析的选择器: " << selector_list->to_string() << std::endl;
            std::cout << "选择器数量: " << selector_list->size() << std::endl;
        } else {
            std::cout << "选择器解析失败" << std::endl;
            return 1;
        }

        // 打印解析错误（如果有）
        const auto& errors = parser.get_errors();
        if (!errors.empty()) {
            std::cout << "解析错误:" << std::endl;
            for (const auto& error : errors) {
                std::cout << "  - " << error.message << std::endl;
            }
        }

        // 使用选择器查找匹配的元素
        std::cout << "\n=== 使用CSS选择器查找匹配元素 ===" << std::endl;
        const auto matched_elements = res->css(".entry-content p > :not(span)").elements();

        std::cout << "匹配的元素数量: " << matched_elements.size() << std::endl;

        for (size_t i = 0; i < matched_elements.size(); ++i) {
            const auto& element = matched_elements[i];
            std::cout << "元素 " << (i + 1) << ":" << std::endl;
            std::cout << "  标签名: " << element->tag_name() << std::endl;
            std::cout << "  文本内容: \"" << element->text_content() << "\"" << std::endl;

            // 打印属性
            if (element->attribute_count() > 0) {
                std::cout << "  属性:" << std::endl;
                for (const auto& attr : element->attributes()) {
                    std::cout << "    " << attr.name() << "=\"" << attr.value() << "\"" << std::endl;
                }
            }
            std::cout << std::endl;
        }

        // 额外测试：手动遍历DOM结构
        std::cout << "=== 手动遍历DOM结构验证 ===" << std::endl;
        const auto p_elements = res->css(".entry-content p").elements();

        for (const auto& p : p_elements) {
            std::cout << "p元素的子元素:" << std::endl;
            const auto children = p->children();

            for (const auto& child : children) {
                if (child->is_element()) {
                    auto elem = child->as_element();
                    std::cout << "  子元素: " << elem->tag_name();

                    // 测试:not(span)逻辑
                    if (elem->tag_name() != "span") {
                        std::cout << " [匹配 :not(span)]";
                    } else {
                        std::cout << " [不匹配 :not(span)]";
                    }

                    std::cout << " - 内容: \"" << elem->text_content() << "\"" << std::endl;
                } else if (child->is_text()) {
                    auto        text    = child->as_text();
                    std::string content = text->value();
                    // 去除前后空白
                    content.erase(0, content.find_first_not_of(" \t\n\r"));
                    content.erase(content.find_last_not_of(" \t\n\r") + 1);

                    if (!content.empty()) {
                        std::cout << "  文本节点: \"" << content << "\"" << std::endl;
                    }
                }
            }
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