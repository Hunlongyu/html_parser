#include "hps/hps.hpp"

#include <chrono>
#include <iostream>
#include <string>

using namespace hps;

int main() {
    // 示例HTML内容
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <title>CSS查询示例</title>
        </head>
        <body>
            <div class="container">
                <h1 id="main-title">CSS选择器测试</h1>
                <div class="content">
                    <p class="intro">这是介绍段落</p>
                    <ul class="list">
                        <li class="item active">第一项</li>
                        <li class="item">第二项</li>
                        <li class="item highlight">第三项</li>
                    </ul>
                    <div class="footer">
                        <span data-role="info">信息</span>
                        <a href="#" class="link">链接</a>
                    </div>
                </div>
            </div>
        </body>
        </html>
    )";

    try {
        // 解析HTML文档
        const auto document = parse(html);
        std::cout << "HTML解析成功!\n\n";

        // 1. 基础选择器测试
        std::cout << "=== 基础选择器测试 ===\n";

        // 标签选择器
        auto paragraphs = document->css("p");
        std::cout << "p标签数量: " << paragraphs.size() << "\n";

        // ID选择器
        auto title = document->css("#main-title");
        if (!title.empty()) {
            std::cout << "标题内容: " << title.first_element()->text_content() << "\n";
        }

        // 类选择器
        auto items = document->css(".item");
        std::cout << ".item类元素数量: " << items.size() << "\n";

        // 2. 属性选择器测试
        std::cout << "\n=== 属性选择器测试 ===\n";

        // 属性存在选择器
        auto dataElements = document->css("[data-role]");
        std::cout << "带data-role属性的元素: " << dataElements.size() << "\n";

        // 属性值选择器
        auto infoElements = document->css("[data-role='info']");
        std::cout << "data-role='info'的元素: " << infoElements.size() << "\n";

        // 3. 组合选择器测试
        std::cout << "\n=== 组合选择器测试 ===\n";

        // 后代选择器
        auto containerPs = document->css(".container p");
        std::cout << ".container内的p元素: " << containerPs.size() << "\n";

        // 子选择器
        auto directItems = document->css(".list > .item");
        std::cout << ".list的直接子.item元素: " << directItems.size() << "\n";

        // 相邻兄弟选择器
        auto adjacentItems = document->css(".item + .item");
        std::cout << ".item相邻的.item元素: " << adjacentItems.size() << "\n";

        // 4. 复合选择器测试
        std::cout << "\n=== 复合选择器测试 ===\n";

        // 多类选择器
        auto activeItems = document->css(".item.active");
        std::cout << ".item.active元素: " << activeItems.size() << "\n";

        // 标签+类选择器
        auto liItems = document->css("li.item");
        std::cout << "li.item元素: " << liItems.size() << "\n";

        // 5. 伪类选择器测试（如果实现）
        std::cout << "\n=== 伪类选择器测试 ===\n";

        // 第一个子元素
        auto firstItems = document->css(".item:first-child");
        std::cout << ":first-child匹配: " << firstItems.size() << "\n";

        // 最后一个子元素
        auto lastItems = document->css(".item:last-child");
        std::cout << ":last-child匹配: " << lastItems.size() << "\n";

        // 6. 链式查询测试
        std::cout << "\n=== 链式查询测试 ===\n";

        auto container = document->css(".container");
        if (!container.empty()) {
            // 在容器内查找
            auto nestedPs = container.css("p");
            std::cout << "容器内p元素: " << nestedPs.size() << "\n";

            // 获取父元素
            auto parents = nestedPs.parent();
            std::cout << "p元素的父元素数量: " << parents.size() << "\n";

            // 获取兄弟元素
            auto siblings = nestedPs.siblings();
            std::cout << "p元素的兄弟元素数量: " << siblings.size() << "\n";
        }

        // 7. 文本内容查询
        std::cout << "\n=== 文本内容查询 ===\n";

        auto allTexts = document->css("*").extract_texts();
        std::cout << "提取的文本数量: " << allTexts.size() << "\n";

        // 查找包含特定文本的元素
        auto introElements = document->css("*").containing_text("介绍");
        std::cout << "包含'介绍'文本的元素: " << introElements.size() << "\n";

        // 8. 性能测试
        std::cout << "\n=== 性能测试 ===\n";

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1000; ++i) {
            document->css(".item");
        }
        auto end      = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "1000次.item查询耗时: " << duration.count() << "微秒\n";

        std::cout << "\n=== CSS查询示例完成 ===\n";

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}