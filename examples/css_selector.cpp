#include "hps/query/css/css_selector.hpp"

#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/parsing/html_parser.hpp"
#include "hps/query/css/css_parser.hpp"

#include <functional>
#include <iostream>
#include <memory>

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
    try {
        // 创建HTML解析器并解析更复杂的HTML结构
        hps::HTMLParser   parser;
        const std::string html = R"(
            <html>
                <body>
                    <div id="main" class="container">
                        <h1>主标题</h1>
                        <p class="text highlight">第一段落</p>
                        <p class="text">第二段落</p>
                        <p class="text special">第三段落</p>
                        <span id="info">信息元素</span>
                        <div class="nested">
                            <p>嵌套段落1</p>
                            <p class="important">嵌套段落2</p>
                            <p>嵌套段落3</p>
                        </div>
                        <ul>
                            <li>列表项1</li>
                            <li class="active">列表项2</li>
                            <li>列表项3</li>
                            <li>列表项4</li>
                        </ul>
                        <form>
                            <input type="text" name="username" />
                            <input type="password" name="password" disabled />
                            <input type="checkbox" name="remember" checked />
                            <button type="submit">提交</button>
                        </form>
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

        std::cout << "=== CSS选择器功能演示 ===" << std::endl;

        // 递归查找匹配元素的辅助函数
        std::function<void(const hps::Element&, const hps::CSSSelector&, const std::string&)> find_and_display;
        find_and_display = [&](const hps::Element& element, const hps::CSSSelector& selector, const std::string& desc) {
            static bool first_match = true;
            if (first_match) {
                std::cout << desc << ":" << std::endl;
                first_match = false;
            }

            if (selector.matches(element)) {
                std::cout << "  匹配: <" << element.tag_name() << ">";
                if (element.has_attribute("class")) {
                    std::cout << " class=\"" << element.get_attribute("class") << "\"";
                }
                if (element.has_attribute("id")) {
                    std::cout << " id=\"" << element.get_attribute("id") << "\"";
                }
                if (element.has_attribute("type")) {
                    std::cout << " type=\"" << element.get_attribute("type") << "\"";
                }
                std::cout << " - 内容: \"" << element.text_content();
                std::cout << "\"" << std::endl;
            }

            for (const auto& child : element.children()) {
                if (child->is_element()) {
                    auto child_element = child->as_element();
                    if (child_element) {
                        find_and_display(*child_element, selector, desc);
                    }
                }
            }
        };

        // 重置匹配标志的辅助函数
        auto reset_and_test = [&](std::unique_ptr<hps::CSSSelector> selector, const std::string& desc) {
            find_and_display(*root, *selector, desc);
            std::cout << std::endl;
        };

        // ========== 基础选择器测试 ==========
        std::cout << "\n========== 基础选择器测试 ==========" << std::endl;

        // 1. 类型选择器
        reset_and_test(std::make_unique<hps::TypeSelector>("p"), "1. 类型选择器 (p)");

        // 2. 类选择器
        reset_and_test(std::make_unique<hps::ClassSelector>("text"), "2. 类选择器 (.text)");

        // 3. ID选择器
        reset_and_test(std::make_unique<hps::IdSelector>("main"), "3. ID选择器 (#main)");

        // 4. 属性选择器
        reset_and_test(std::make_unique<hps::AttributeSelector>("class", hps::AttributeOperator::Contains, "highlight"), "4. 属性选择器 ([class*=\"highlight\"])");

        // ========== 伪类选择器测试 ==========
        std::cout << "\n========== 伪类选择器测试 ==========" << std::endl;

        // 5. :first-child 伪类
        reset_and_test(std::make_unique<hps::PseudoClassSelector>(hps::PseudoClassSelector::PseudoType::FirstChild), "5. :first-child 伪类");

        // 6. :last-child 伪类
        reset_and_test(std::make_unique<hps::PseudoClassSelector>(hps::PseudoClassSelector::PseudoType::LastChild), "6. :last-child 伪类");

        // 7. :nth-child(2) 伪类
        reset_and_test(std::make_unique<hps::PseudoClassSelector>(hps::PseudoClassSelector::PseudoType::NthChild, "2"), "7. :nth-child(2) 伪类");

        // 8. :nth-child(odd) 伪类
        reset_and_test(std::make_unique<hps::PseudoClassSelector>(hps::PseudoClassSelector::PseudoType::NthChild, "odd"), "8. :nth-child(odd) 伪类");

        // 9. :only-child 伪类
        reset_and_test(std::make_unique<hps::PseudoClassSelector>(hps::PseudoClassSelector::PseudoType::OnlyChild), "9. :only-child 伪类");

        // 10. :empty 伪类
        reset_and_test(std::make_unique<hps::PseudoClassSelector>(hps::PseudoClassSelector::PseudoType::Empty), "10. :empty 伪类");

        // 11. :root 伪类
        reset_and_test(std::make_unique<hps::PseudoClassSelector>(hps::PseudoClassSelector::PseudoType::Root), "11. :root 伪类");

        // 12. :disabled 伪类
        reset_and_test(std::make_unique<hps::PseudoClassSelector>(hps::PseudoClassSelector::PseudoType::Disabled), "12. :disabled 伪类");

        // 13. :enabled 伪类
        reset_and_test(std::make_unique<hps::PseudoClassSelector>(hps::PseudoClassSelector::PseudoType::Enabled), "13. :enabled 伪类");

        // 14. :checked 伪类
        reset_and_test(std::make_unique<hps::PseudoClassSelector>(hps::PseudoClassSelector::PseudoType::Checked), "14. :checked 伪类");

        // ========== 兄弟选择器测试 ==========
        std::cout << "\n========== 兄弟选择器测试 ==========" << std::endl;

        // 15. 相邻兄弟选择器 h1 + p
        auto adjacent_sibling = std::make_unique<hps::AdjacentSiblingSelector>(std::make_unique<hps::TypeSelector>("h1"), std::make_unique<hps::TypeSelector>("p"));
        reset_and_test(std::move(adjacent_sibling), "15. 相邻兄弟选择器 (h1 + p)");

        // 16. 通用兄弟选择器 h1 ~ p
        auto general_sibling = std::make_unique<hps::GeneralSiblingSelector>(std::make_unique<hps::TypeSelector>("h1"), std::make_unique<hps::TypeSelector>("p"));
        reset_and_test(std::move(general_sibling), "16. 通用兄弟选择器 (h1 ~ p)");

        // 17. 类选择器 + 相邻兄弟选择器 .highlight + p
        auto class_adjacent = std::make_unique<hps::AdjacentSiblingSelector>(std::make_unique<hps::ClassSelector>("highlight"), std::make_unique<hps::TypeSelector>("p"));
        reset_and_test(std::move(class_adjacent), "17. 类 + 相邻兄弟选择器 (.highlight + p)");

        // ========== 复合选择器测试 ==========
        std::cout << "\n========== 复合选择器测试 ==========" << std::endl;

        // 18. 复合选择器 p.text
        auto compound_selector = std::make_unique<hps::CompoundSelector>();
        compound_selector->add_selector(std::make_unique<hps::TypeSelector>("p"));
        compound_selector->add_selector(std::make_unique<hps::ClassSelector>("text"));
        reset_and_test(std::move(compound_selector), "18. 复合选择器 (p.text)");

        // 19. 复合选择器 + 伪类 li:first-child
        auto compound_pseudo = std::make_unique<hps::CompoundSelector>();
        compound_pseudo->add_selector(std::make_unique<hps::TypeSelector>("li"));
        compound_pseudo->add_selector(std::make_unique<hps::PseudoClassSelector>(hps::PseudoClassSelector::PseudoType::FirstChild));
        reset_and_test(std::move(compound_pseudo), "19. 复合选择器 + 伪类 (li:first-child)");

        // 20. 复合选择器 + nth-child input:nth-child(2)
        auto compound_nth = std::make_unique<hps::CompoundSelector>();
        compound_nth->add_selector(std::make_unique<hps::TypeSelector>("input"));
        compound_nth->add_selector(std::make_unique<hps::PseudoClassSelector>(hps::PseudoClassSelector::PseudoType::NthChild, "2"));
        reset_and_test(std::move(compound_nth), "20. 复合选择器 + nth-child (input:nth-child(2))");

        // ========== 组合器选择器测试 ==========
        std::cout << "\n========== 组合器选择器测试 ==========" << std::endl;

        // 21. 后代选择器 div p
        auto descendant_selector = std::make_unique<hps::DescendantSelector>(std::make_unique<hps::TypeSelector>("div"), std::make_unique<hps::TypeSelector>("p"));
        reset_and_test(std::move(descendant_selector), "21. 后代选择器 (div p)");

        // 22. 子选择器 div > p
        auto child_selector = std::make_unique<hps::ChildSelector>(std::make_unique<hps::TypeSelector>("div"), std::make_unique<hps::TypeSelector>("p"));
        reset_and_test(std::move(child_selector), "22. 子选择器 (div > p)");

        // ========== 选择器优先级和字符串化测试 ==========
        std::cout << "\n========== 选择器优先级和字符串化测试 ==========" << std::endl;

        // 创建不同类型的选择器用于测试
        auto test_type   = std::make_unique<hps::TypeSelector>("p");
        auto test_class  = std::make_unique<hps::ClassSelector>("text");
        auto test_id     = std::make_unique<hps::IdSelector>("main");
        auto test_pseudo = std::make_unique<hps::PseudoClassSelector>(hps::PseudoClassSelector::PseudoType::FirstChild);
        auto test_attr   = std::make_unique<hps::AttributeSelector>("class", hps::AttributeOperator::Equals, "highlight");

        // 测试优先级计算
        std::cout << "\n23. 选择器优先级测试:" << std::endl;
        auto spec1 = test_type->calculate_specificity();
        auto spec2 = test_class->calculate_specificity();
        auto spec3 = test_id->calculate_specificity();
        auto spec4 = test_pseudo->calculate_specificity();
        auto spec5 = test_attr->calculate_specificity();

        std::cout << "  类型选择器 (p): elements=" << spec1.elements << ", classes=" << spec1.classes << ", ids=" << spec1.ids << std::endl;
        std::cout << "  类选择器 (.text): elements=" << spec2.elements << ", classes=" << spec2.classes << ", ids=" << spec2.ids << std::endl;
        std::cout << "  ID选择器 (#main): elements=" << spec3.elements << ", classes=" << spec3.classes << ", ids=" << spec3.ids << std::endl;
        std::cout << "  伪类选择器 (:first-child): elements=" << spec4.elements << ", classes=" << spec4.classes << ", ids=" << spec4.ids << std::endl;
        std::cout << "  属性选择器 ([class=\"highlight\"]): elements=" << spec5.elements << ", classes=" << spec5.classes << ", ids=" << spec5.ids << std::endl;

        // 测试字符串化
        std::cout << "\n24. 选择器字符串化测试:" << std::endl;
        std::cout << "  类型选择器: " << test_type->to_string() << std::endl;
        std::cout << "  类选择器: " << test_class->to_string() << std::endl;
        std::cout << "  ID选择器: " << test_id->to_string() << std::endl;
        std::cout << "  伪类选择器: " << test_pseudo->to_string() << std::endl;
        std::cout << "  属性选择器: " << test_attr->to_string() << std::endl;

        // 复合选择器字符串化
        auto compound_str_test = std::make_unique<hps::CompoundSelector>();
        compound_str_test->add_selector(std::make_unique<hps::TypeSelector>("input"));
        compound_str_test->add_selector(std::make_unique<hps::AttributeSelector>("type", hps::AttributeOperator::Equals, "text"));
        compound_str_test->add_selector(std::make_unique<hps::PseudoClassSelector>(hps::PseudoClassSelector::PseudoType::Enabled));
        std::cout << "  复合选择器: " << compound_str_test->to_string() << std::endl;

        // 兄弟选择器字符串化
        auto sibling_str_test = std::make_unique<hps::AdjacentSiblingSelector>(std::make_unique<hps::ClassSelector>("highlight"), std::make_unique<hps::TypeSelector>("span"));
        std::cout << "  相邻兄弟选择器: " << sibling_str_test->to_string() << std::endl;

        std::cout << "\n=== 测试完成 ===" << std::endl;
        std::cout << "\n新功能演示:" << std::endl;
        std::cout << "✓ 伪类选择器: :first-child, :last-child, :nth-child, :empty, :root, :disabled, :enabled, :checked" << std::endl;
        std::cout << "✓ 兄弟选择器: 相邻兄弟选择器 (+), 通用兄弟选择器 (~)" << std::endl;
        std::cout << "✓ 复合选择器: 支持多个简单选择器组合" << std::endl;
        std::cout << "✓ 选择器优先级: 正确计算CSS特异性" << std::endl;
        std::cout << "✓ 字符串化: 所有选择器都支持转换为CSS字符串" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    system("pause");
    return 0;
}