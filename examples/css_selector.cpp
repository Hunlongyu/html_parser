#include "hps/query/css/css_selector.hpp"

#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/parsing/html_parser.hpp"

#include <iostream>
#include <memory>

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
    try {
        // 创建HTML解析器并解析HTML
        hps::HTMLParser   parser;
        const std::string html = R"(
            <html>
                <body>
                    <div id="main" class="container">
                        <p class="text highlight">段落1</p>
                        <p class="text">段落2</p>
                        <span id="info">信息</span>
                        <div class="nested">
                            <p>嵌套段落</p>
                        </div>
                    </div>
                </body>
            </html>
        )";

        auto document = parser.parse(html);
        auto root     = document->root();

        if (!root) {
            std::cerr << "无法获取根元素" << std::endl;
            return 1;
        }

        std::cout << "=== CSS选择器测试 ===" << std::endl;

        // 1. 测试类型选择器
        std::cout << "\n1. 类型选择器测试:" << std::endl;
        auto type_selector = std::make_unique<hps::TypeSelector>("p");

        // 递归查找所有p元素
        std::function<void(const hps::Element&, const hps::CSSSelector&)> find_matching;
        find_matching = [&](const hps::Element& element, const hps::CSSSelector& selector) {
            if (selector.matches(element)) {
                std::cout << "匹配: <" << element.tag_name() << ">";
                if (element.has_attribute("class")) {
                    std::cout << " class=\"" << element.get_attribute("class") << "\"";
                }
                if (element.has_attribute("id")) {
                    std::cout << " id=\"" << element.get_attribute("id") << "\"";
                }
                std::cout << std::endl;
            }

            for (const auto& child : element.children()) {
                if (child->is_element()) {
                    auto child_element = child->as_element();
                    if (child_element) {
                        find_matching(*child_element, selector);
                    }
                }
            }
        };

        find_matching(*root, *type_selector);

        // 2. 测试类选择器
        std::cout << "\n2. 类选择器测试:" << std::endl;
        auto class_selector = std::make_unique<hps::ClassSelector>("text");
        find_matching(*root, *class_selector);

        // 3. 测试ID选择器
        std::cout << "\n3. ID选择器测试:" << std::endl;
        auto id_selector = std::make_unique<hps::IdSelector>("main");
        find_matching(*root, *id_selector);

        // 4. 测试属性选择器
        std::cout << "\n4. 属性选择器测试:" << std::endl;
        auto attr_selector = std::make_unique<hps::AttributeSelector>("class", hps::AttributeOperator::Contains, "highlight");
        find_matching(*root, *attr_selector);

        // 5. 测试复合选择器
        std::cout << "\n5. 复合选择器测试 (p.text):" << std::endl;
        auto compound_selector = std::make_unique<hps::CompoundSelector>();
        compound_selector->add_selector(std::make_unique<hps::TypeSelector>("p"));
        compound_selector->add_selector(std::make_unique<hps::ClassSelector>("text"));
        find_matching(*root, *compound_selector);

        // 6. 测试选择器优先级
        std::cout << "\n6. 选择器优先级测试:" << std::endl;
        auto specificity1 = type_selector->calculate_specificity();
        auto specificity2 = class_selector->calculate_specificity();
        auto specificity3 = id_selector->calculate_specificity();

        std::cout << "类型选择器优先级: " << specificity1.elements << std::endl;
        std::cout << "类选择器优先级: " << specificity2.classes << std::endl;
        std::cout << "ID选择器优先级: " << specificity3.ids << std::endl;

        // 7. 测试选择器字符串化
        std::cout << "\n7. 选择器字符串化测试:" << std::endl;
        std::cout << "类型选择器: " << type_selector->to_string() << std::endl;
        std::cout << "类选择器: " << class_selector->to_string() << std::endl;
        std::cout << "ID选择器: " << id_selector->to_string() << std::endl;
        std::cout << "属性选择器: " << attr_selector->to_string() << std::endl;
        std::cout << "复合选择器: " << compound_selector->to_string() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    system("pause");
    return 0;
}