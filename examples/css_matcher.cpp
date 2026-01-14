#include "hps/query/css/css_matcher.hpp"

#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/parsing/html_parser.hpp"
#include "hps/query/css/css_parser.hpp"
#include "hps/query/css/css_utils.hpp"

#include <iostream>
#include <memory>
#include <vector>

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    try {
        // 创建HTML解析器并解析HTML文档
        hps::HTMLParser   parser;
        const std::string html = R"(
            <html>
                <head>
                    <title>CSS Matcher 示例</title>
                </head>
                <body>
                    <div id="container" class="main-content">
                        <h1>标题</h1>
                        <p class="intro highlight">介绍段落</p>
                        <p class="content">内容段落1</p>
                        <p class="content special">内容段落2</p>
                        <p class="中文类名">内容段落3</p>
                        <div class="sidebar">
                            <h2>侧边栏标题</h2>
                            <ul>
                                <li class="item">项目1</li>
                                <li class="item active">项目2</li>
                                <li class="item">项目3</li>
                            </ul>
                        </div>
                        <form>
                            <input type="text" name="username" placeholder="用户名" />
                            <input type="password" name="password" disabled />
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

        std::cout << "=== CSS Matcher 功能演示 ===" << std::endl;

        // 辅助函数：显示匹配结果
        auto display_results = [](const std::vector<const hps::Element*>& results, const std::string& selector_desc) {
            std::cout << "\n" << selector_desc << " (找到 " << results.size() << " 个匹配):" << std::endl;
            for (const auto& element : results) {
                std::cout << "  - <" << element->tag_name() << ">";
                if (element->has_attribute("id")) {
                    std::cout << " id=\"" << element->get_attribute("id") << "\"";
                }
                if (element->has_attribute("class")) {
                    std::cout << " class=\"" << element->get_attribute("class") << "\"";
                }
                std::cout << " - 内容: \"" << element->text_content() << "\"" << std::endl;
            }
        };

        // ========== 基础选择器测试 ==========
        std::cout << "\n========== 基础选择器测试 ==========" << std::endl;

        // 1. 类型选择器 - 查找所有 p 元素
        {
            auto selector_list = hps::parse_css_selector("p");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "1. 类型选择器 'p'");
        }

        // 2. 类选择器 - 查找所有 .content 类的元素
        {
            auto selector_list = hps::parse_css_selector(".content");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "2. 类选择器 '.content'");
        }

        // 3. ID选择器 - 查找 #container 元素
        {
            auto selector_list = hps::parse_css_selector("#container");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "3. ID选择器 '#container'");
        }

        // 4. 属性选择器 - 查找有 disabled 属性的元素
        {
            auto selector_list = hps::parse_css_selector("[disabled]");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "4. 属性选择器 '[disabled]'");
        }

        // 5. 通用选择器 - 查找所有元素
        {
            auto selector_list = hps::parse_css_selector("*");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            std::cout << "\n5. 通用选择器 '*' (找到 " << results.size() << " 个匹配)" << std::endl;
            std::cout << "  (显示前5个结果)" << std::endl;
            for (size_t i = 0; i < std::min(results.size(), size_t(5)); ++i) {
                const auto& element = results[i];
                std::cout << "  - <" << element->tag_name() << ">";
                if (element->has_attribute("id")) {
                    std::cout << " id=\"" << element->get_attribute("id") << "\"";
                }
                if (element->has_attribute("class")) {
                    std::cout << " class=\"" << element->get_attribute("class") << "\"";
                }
                std::cout << std::endl;
            }
        }

        // ========== 组合器选择器测试 ==========
        std::cout << "\n========== 组合器选择器测试 ==========" << std::endl;

        // 6. 后代选择器 - 查找 div 内的所有 p 元素
        {
            auto selector_list = hps::parse_css_selector("div p");
            std::cout << "Parsed selector: " << selector_list->to_string() << std::endl;  // 调试输出

            auto results = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "6. 后代选择器 'div p'");
        }

        // 7. 子选择器 - 查找 div 的直接子元素 p
        {
            auto selector_list = hps::parse_css_selector("div > p");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "7. 子选择器 'div > p'");
        }

        // 8. 相邻兄弟选择器 - 查找 h1 后面紧邻的 p 元素
        {
            auto selector_list = hps::parse_css_selector("h1 + p");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "8. 相邻兄弟选择器 'h1 + p'");
        }

        // 9. 通用兄弟选择器 - 查找 h1 后面的所有 p 兄弟元素
        {
            auto selector_list = hps::parse_css_selector("h1 ~ p");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "9. 通用兄弟选择器 'h1 ~ p'");
        }

        // ========== 复合选择器测试 ==========
        std::cout << "\n========== 复合选择器测试 ==========" << std::endl;

        // 10. 复合选择器 - 查找同时具有 content 和 special 类的 p 元素
        {
            auto selector_list = hps::parse_css_selector("p.content.special");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "10. 复合选择器 'p.content.special'");
        }

        // 11. 复合选择器 - 查找类型为 text 的 input 元素
        {
            auto selector_list = hps::parse_css_selector("input[type=\"text\"]");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "11. 复合选择器 'input[type=\"text\"]'");
        }

        // 12. 复合选择器 - ID和类组合
        {
            auto selector_list = hps::parse_css_selector("#container.main-content");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "12. 复合选择器 '#container.main-content'");
        }

        // ========== 伪类选择器测试 ==========
        std::cout << "\n========== 伪类选择器测试 ==========" << std::endl;

        // 13. :first-child 伪类
        {
            auto selector_list = hps::parse_css_selector("li:first-child");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "13. 伪类选择器 'li:first-child'");
        }

        // 14. :last-child 伪类
        {
            auto selector_list = hps::parse_css_selector("li:last-child");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "14. 伪类选择器 'li:last-child'");
        }

        // 15. :disabled 伪类
        {
            auto selector_list = hps::parse_css_selector(":disabled");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "15. 伪类选择器 ':disabled'");
        }

        // 16. :nth-child 伪类
        {
            auto selector_list = hps::parse_css_selector("li:nth-child(2)");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "16. 伪类选择器 'li:nth-child(2)'");
        }

        // ========== 单个元素匹配测试 ==========
        std::cout << "\n========== 单个元素匹配测试 ==========" << std::endl;

        // 17. 查找第一个匹配的元素
        {
            auto selector_list = hps::parse_css_selector(".item");
            auto first_result  = hps::CSSMatcher::find_first(*document, *selector_list);
            if (first_result) {
                std::cout << "\n17. 查找第一个 '.item' 元素:" << std::endl;
                std::cout << "  - <" << first_result->tag_name() << ">";
                if (first_result->has_attribute("class")) {
                    std::cout << " class=\"" << first_result->get_attribute("class") << "\"";
                }
                std::cout << " - 内容: \"" << first_result->text_content() << "\"" << std::endl;
            }
        }

        // ========== 元素匹配检查测试 ==========
        std::cout << "\n========== 元素匹配检查测试 ==========" << std::endl;

        // 18. 检查特定元素是否匹配选择器
        {
            auto container = hps::CSSMatcher::find_first(*document, *hps::parse_css_selector("#container"));
            if (container) {
                auto selector_list = hps::parse_css_selector(".main-content");
                bool matches       = selector_list->matches(*container);
                std::cout << "\n18. #container 元素是否匹配 '.main-content': " << (matches ? "是" : "否") << std::endl;

                auto div_selector = hps::parse_css_selector("div");
                bool matches_div  = div_selector->matches(*container);
                std::cout << "    #container 元素是否匹配 'div': " << (matches_div ? "是" : "否") << std::endl;
            }
        }

        // ========== 复杂选择器组合测试 ==========
        std::cout << "\n========== 复杂选择器组合测试 ==========" << std::endl;

        // 19. 多个选择器组合（选择器列表）
        {
            auto selector_list = hps::parse_css_selector("h1, h2, .highlight");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "19. 选择器列表 'h1, h2, .highlight'");
        }

        // 20. 复杂的后代选择器组合
        {
            auto selector_list = hps::parse_css_selector(".sidebar ul li.active");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "20. 复杂后代选择器 '.sidebar ul li.active'");
        }

        // 21. 属性值匹配
        {
            auto selector_list = hps::parse_css_selector("input[name=\"username\"]");
            auto results       = hps::CSSMatcher::find_all(*document, *selector_list);
            display_results(results, "21. 属性值选择器 'input[name=\"username\"]'");
        }

        // ========== 从特定元素开始查找测试 ==========
        std::cout << "\n========== 从特定元素开始查找测试 ==========" << std::endl;

        // 22. 从sidebar元素开始查找
        {
            auto sidebar = hps::CSSMatcher::find_first(*document, *hps::parse_css_selector(".sidebar"));
            if (sidebar) {
                auto selector_list = hps::parse_css_selector("li");
                auto results       = hps::CSSMatcher::find_all(*sidebar, *selector_list);
                display_results(results, "22. 从 .sidebar 元素开始查找 'li'");
            }
        }

        std::cout << "\n=== CSS Matcher 演示完成 ===" << std::endl;
        std::cout << "\n功能特性:" << std::endl;
        std::cout << "✓ 支持所有基础CSS选择器类型（类型、类、ID、属性、通用）" << std::endl;
        std::cout << "✓ 支持组合器选择器（后代、子、相邻兄弟、通用兄弟）" << std::endl;
        std::cout << "✓ 支持复合选择器和选择器列表" << std::endl;
        std::cout << "✓ 支持伪类选择器（:first-child, :last-child, :nth-child, :disabled等）" << std::endl;
        std::cout << "✓ 提供查找所有匹配元素和查找第一个匹配元素的接口" << std::endl;
        std::cout << "✓ 提供元素匹配检查功能" << std::endl;
        std::cout << "✓ 支持从Document或Element根节点开始查找" << std::endl;
        std::cout << "✓ 支持属性值精确匹配" << std::endl;
        std::cout << "✓ 支持复杂的选择器组合" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

#ifdef _WIN32
    system("pause");
#endif
    return 0;
}
