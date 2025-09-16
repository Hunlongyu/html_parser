#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/hps.hpp"
#include "hps/query/element_query.hpp"
#include "hps/query/query.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace hps;

// 测试用的HTML内容
const std::string test_html = R"(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <title>CSS Query Test</title>
</head>
<body>
    <div class="container" id="main">
        <header class="header">
            <h1 class="title">主标题</h1>
            <nav class="navigation">
                <ul class="menu">
                    <li class="item active"><a href="#home">首页</a></li>
                    <li class="item"><a href="#about">关于</a></li>
                    <li class="item"><a href="#contact">联系</a></li>
                </ul>
            </nav>
        </header>
        
        <main class="content">
            <section class="intro">
                <p class="description">这是一个测试页面。</p>
                <p class="highlight">重要内容在这里。</p>
            </section>
            
            <section class="features">
                <div class="feature" data-type="primary">
                    <h2>功能一</h2>
                    <p>功能一的描述</p>
                </div>
                <div class="feature" data-type="secondary">
                    <h2>功能二</h2>
                    <p>功能二的描述</p>
                </div>
                <div class="feature disabled" data-type="tertiary">
                    <h2>功能三</h2>
                    <p>功能三的描述</p>
                </div>
            </section>
            
            <form class="contact-form">
                <input type="text" name="name" placeholder="姓名" required>
                <input type="email" name="email" placeholder="邮箱" required>
                <textarea name="message" placeholder="消息" disabled></textarea>
                <button type="submit" class="btn btn-primary">提交</button>
            </form>
        </main>
        
        <aside class="sidebar">
            <div class="widget">
                <h3>相关链接</h3>
                <ul>
                    <li><a href="https://example.com">外部链接</a></li>
                    <li><a href="/internal">内部链接</a></li>
                </ul>
            </div>
        </aside>
        
        <footer class="footer">
            <p>&copy; 2024 测试网站</p>
        </footer>
    </div>
</body>
</html>
)";

// 辅助函数：打印查询结果
void print_query_results(const ElementQuery& results, const std::string& selector) {
    std::cout << "\n选择器: \"" << selector << "\"" << std::endl;
    std::cout << "匹配结果: " << results.size() << " 个元素" << std::endl;

    if (!results.empty()) {
        std::cout << "元素详情:" << std::endl;
        for (size_t i = 0; i < results.size(); ++i) {
            auto element = results[i];
            if (element) {
                std::cout << "  [" << i << "] <" << element->tag_name();

                // 显示ID
                if (element->has_attribute("id")) {
                    std::cout << " id=\"" << element->get_attribute("id") << "\"";
                }

                // 显示class
                if (element->has_attribute("class")) {
                    std::cout << " class=\"" << element->get_attribute("class") << "\"";
                }

                // 显示其他重要属性
                if (element->has_attribute("data-type")) {
                    std::cout << " data-type=\"" << element->get_attribute("data-type") << "\"";
                }
                if (element->has_attribute("type")) {
                    std::cout << " type=\"" << element->get_attribute("type") << "\"";
                }

                std::cout << ">";

                // 显示文本内容（如果有且不太长）
                auto text = element->text_content();
                if (!text.empty() && text.length() < 50) {
                    std::cout << text;
                }

                std::cout << "</" << element->tag_name() << ">" << std::endl;
            }
        }
    }
}

// 测试基本选择器
void test_basic_selectors(const std::shared_ptr<Document>& doc) {
    std::cout << "\n=== 基本选择器测试 ===" << std::endl;

    std::vector<std::string> basic_selectors = {
        "div",         // 标签选择器
        ".container",  // 类选择器
        "#main",       // ID选择器
        "*"            // 通用选择器
    };

    for (const auto& selector : basic_selectors) {
        auto results = Query::css(*doc, selector);
        print_query_results(results, selector);
    }
}

// 测试属性选择器
void test_attribute_selectors(const std::shared_ptr<Document>& doc) {
    std::cout << "\n=== 属性选择器测试 ===" << std::endl;

    std::vector<std::string> attr_selectors = {
        "[data-type]",               // 存在属性
        "[type=\"text\"]",           // 属性值完全匹配
        "[class*=\"btn\"]",          // 属性值包含
        "[href^=\"https\"]",         // 属性值开头匹配
        "[name$=\"name\"]",          // 属性值结尾匹配
        "[data-type|=\"primary\"]",  // 属性值语言匹配
        "[class~=\"feature\"]",      // 属性值单词匹配
    };

    for (const auto& selector : attr_selectors) {
        auto results = Query::css(*doc, selector);
        print_query_results(results, selector);
    }
}

// 测试组合选择器
void test_combinator_selectors(const std::shared_ptr<Document>& doc) {
    std::cout << "\n=== 组合选择器测试 ===" << std::endl;

    std::vector<std::string> combinator_selectors = {
        "div p",          // 后代选择器
        "nav > ul",       // 子选择器
        "h1 + nav",       // 相邻兄弟选择器
        "header ~ main",  // 通用兄弟选择器
    };

    for (const auto& selector : combinator_selectors) {
        auto results = Query::css(*doc, selector);
        print_query_results(results, selector);
    }
}

// 测试复合选择器
void test_compound_selectors(const std::shared_ptr<Document>& doc) {
    std::cout << "\n=== 复合选择器测试 ===" << std::endl;

    std::vector<std::string> compound_selectors = {
        "div.container#main",             // 标签+类+ID
        "input[type=\"text\"].required",  // 标签+属性+类（假设有required类）
        "button.btn.btn-primary",         // 标签+多个类
        "div.feature[data-type]",         // 标签+类+属性
    };

    for (const auto& selector : compound_selectors) {
        auto results = Query::css(*doc, selector);
        print_query_results(results, selector);
    }
}

// 测试伪类选择器
void test_pseudo_selectors(const std::shared_ptr<Document>& doc) {
    std::cout << "\n=== 伪类选择器测试 ===" << std::endl;

    std::vector<std::string> pseudo_selectors = {
        "li:first-child",      // 第一个子元素
        "li:last-child",       // 最后一个子元素
        "div:nth-child(2)",    // 第n个子元素
        "p:nth-child(odd)",    // 奇数子元素
        "input:disabled",      // 禁用状态
        "input:required",      // 必填状态
        "div:not(.disabled)",  // 非匹配
        "section:empty",       // 空元素（可能没有）
    };

    for (const auto& selector : pseudo_selectors) {
        auto results = Query::css(*doc, selector);
        print_query_results(results, selector);
    }
}

// 测试选择器列表
void test_selector_lists(const std::shared_ptr<Document>& doc) {
    std::cout << "\n=== 选择器列表测试 ===" << std::endl;

    std::vector<std::string> list_selectors = {
        "h1, h2, h3",                      // 多个标签
        ".title, .description",            // 多个类
        "#main, .container",               // ID和类混合
        "input[type=\"text\"], textarea",  // 属性选择器和标签混合
    };

    for (const auto& selector : list_selectors) {
        auto results = Query::css(*doc, selector);
        print_query_results(results, selector);
    }
}

// 测试复杂选择器
void test_complex_selectors(const std::shared_ptr<Document>& doc) {
    std::cout << "\n=== 复杂选择器测试 ===" << std::endl;

    std::vector<std::string> complex_selectors = {
        "nav ul li a",                                  // 多层后代
        "div.container > main.content p",               // 子选择器+后代选择器
        "section.features div.feature:not(.disabled)",  // 复合+伪类
        "form input[type=\"text\"]:required",           // 属性+伪类
        "ul.menu li.item:first-child a",                // 多层复合选择器
    };

    for (const auto& selector : complex_selectors) {
        auto results = Query::css(*doc, selector);
        print_query_results(results, selector);
    }
}

// 测试元素级查询（从特定元素开始查询）
void test_element_queries(const std::shared_ptr<Document>& doc) {
    std::cout << "\n=== 元素级查询测试 ===" << std::endl;

    // 先找到容器元素
    auto container_results = Query::css(*doc, ".container");
    if (!container_results.empty()) {
        auto container = container_results.first_element();
        std::cout << "\n从容器元素开始查询:" << std::endl;

        std::vector<std::string> element_selectors = {
            "header",           // 在容器内查找header
            ".feature",         // 在容器内查找feature类
            "input, textarea",  // 在容器内查找表单元素
            "a[href^=\"#\"]",   // 在容器内查找内部链接
        };

        for (const auto& selector : element_selectors) {
            auto results = Query::css(*container, selector);
            print_query_results(results, "容器内: " + selector);
        }
    }
}

// 测试链式查询
void test_chained_queries(const std::shared_ptr<Document>& doc) {
    std::cout << "\n=== 链式查询测试 ===" << std::endl;

    // 演示ElementQuery的链式方法
    std::cout << "\n链式查询示例:" << std::endl;

    // 查找所有div，然后筛选有class属性的
    auto divs_with_class = Query::css(*doc, "div").has_attribute("class");
    std::cout << "有class属性的div: " << divs_with_class.size() << " 个" << std::endl;

    // 查找所有input，然后筛选text类型的
    auto text_inputs = Query::css(*doc, "input").has_attribute("type", "text");
    std::cout << "text类型的input: " << text_inputs.size() << " 个" << std::endl;

    // 查找所有p元素，然后获取包含特定文本的
    auto paragraphs_with_text = Query::css(*doc, "p").containing_text("功能");
    std::cout << "包含'功能'文本的p元素: " << paragraphs_with_text.size() << " 个" << std::endl;

    // 查找feature类，然后获取前两个
    auto first_two_features = Query::css(*doc, ".feature").first(2);
    std::cout << "前两个feature元素: " << first_two_features.size() << " 个" << std::endl;
}

// 测试数据提取
void test_data_extraction(const std::shared_ptr<Document>& doc) {
    std::cout << "\n=== 数据提取测试 ===" << std::endl;

    // 提取所有链接的href属性
    auto links = Query::css(*doc, "a");
    auto hrefs = links.extract_attributes("href");
    std::cout << "\n所有链接的href属性:" << std::endl;
    for (size_t i = 0; i < hrefs.size(); ++i) {
        std::cout << "  [" << i << "] " << hrefs[i] << std::endl;
    }

    // 提取所有h1-h3的文本内容
    auto headings      = Query::css(*doc, "h1, h2, h3");
    auto heading_texts = headings.extract_texts();
    std::cout << "\n所有标题文本:" << std::endl;
    for (size_t i = 0; i < heading_texts.size(); ++i) {
        std::cout << "  [" << i << "] " << heading_texts[i] << std::endl;
    }

    // 提取所有feature的data-type属性
    auto features   = Query::css(*doc, ".feature");
    auto data_types = features.extract_attributes("data-type");
    std::cout << "\n所有feature的data-type属性:" << std::endl;
    for (size_t i = 0; i < data_types.size(); ++i) {
        std::cout << "  [" << i << "] " << data_types[i] << std::endl;
    }
}

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    std::cout << "CSS Query Example and Test Suite" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "演示 query.hpp 中 css 方法的使用" << std::endl;

    try {
        // 解析HTML文档
        auto doc = hps::parse(test_html);
        if (!doc) {
            std::cerr << "Failed to parse HTML document" << std::endl;
            return 1;
        }

        std::cout << "\nHTML文档解析成功!" << std::endl;
        std::cout << "文档标题: " << doc->title() << std::endl;

        // 运行各种测试
        test_basic_selectors(doc);
        test_attribute_selectors(doc);
        test_combinator_selectors(doc);
        test_compound_selectors(doc);
        test_pseudo_selectors(doc);
        test_selector_lists(doc);
        test_complex_selectors(doc);
        test_element_queries(doc);
        test_chained_queries(doc);
        test_data_extraction(doc);

        std::cout << "\n=== 所有测试完成 ===" << std::endl;
        std::cout << "\n主要功能演示:" << std::endl;
        std::cout << "• Query::css(document, selector) - 从文档查询" << std::endl;
        std::cout << "• Query::css(element, selector) - 从元素查询" << std::endl;
        std::cout << "• ElementQuery 链式方法调用" << std::endl;
        std::cout << "• 数据提取和文本处理" << std::endl;
        std::cout << "• 各种CSS选择器支持" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\n错误: " << e.what() << std::endl;
        return 1;
    }

#ifdef _WIN32
    system("pause");
#endif
    return 0;
}