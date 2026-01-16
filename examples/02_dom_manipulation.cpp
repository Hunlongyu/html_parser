/**
 * @file 02_dom_manipulation.cpp
 * @brief DOM 操作与属性访问示例 / DOM Manipulation and Attribute Access Example
 *
 * 本示例展示了如何访问和操作 DOM 节点的信息：
 * 1. 查找特定 ID 的元素
 * 2. 查找特定标签名的元素列表
 * 3. 获取和检查元素属性（class, id, custom attributes）
 * 4. 获取元素的文本内容
 *
 * This example demonstrates how to access and manipulate DOM node information:
 * 1. Finding an element by ID
 * 2. Finding elements by tag name
 * 3. Getting and checking element attributes (class, id, custom attributes)
 * 4. Getting element text content
 */

#include "hps/core/element.hpp"
#include "hps/hps.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>

int main() {
    const std::string html = R"(
        <html>
        <body>
            <div id="container" class="main-box" data-type="wrapper">
                <h1 class="title">Product List</h1>
                <ul class="items">
                    <li class="item" data-id="101">Apple <span class="price">$1.00</span></li>
                    <li class="item" data-id="102">Banana <span class="price">$0.50</span></li>
                    <li class="item" data-id="103">Cherry <span class="price">$2.00</span></li>
                </ul>
                <a href="https://example.com" title="More Info">Read more</a>
            </div>
        </body>
        </html>
    )";

    // 解析 HTML / Parse HTML
    const auto doc = hps::parse(html);

    // 1. 获取根节点 / Get root node
    // 通常 Document 的第一个子元素是 <html> (如果存在)
    // Usually the first child of Document is <html> (if exists)
    // 但我们可以直接从 Document 开始查找
    // But we can search directly from Document

    // 2. 按 ID 查找元素 / Find element by ID
    // 注意：Document 类本身可能没有 get_element_by_id，我们需要从 root 元素或使用查询
    // Note: Document class might not have get_element_by_id, we need to search from root or use query
    // 这里我们先获取 body 元素 (简化处理，实际中应该遍历查找 body)
    // Here we get body element first (simplified, in practice should traverse to find body)

    // 为了演示方便，我们假设结构已知。
    // For demonstration, we assume the structure is known.
    // hps::Document 本身也是 Node，可以有 children

    // 使用 querySelector 更方便地定位 / Use querySelector for easier positioning
    const auto* container = doc->get_element_by_id("container");

    if (container) {
        std::cout << "Found container with ID: " << container->id() << '\n';

        // 3. 访问属性 / Access attributes
        if (container->has_attribute("class")) {
            std::cout << "Class: " << container->class_name() << '\n';
        }

        if (container->has_attribute("data-type")) {
            std::cout << "Data Type: " << container->get_attribute("data-type") << '\n';
        }

        // 4. 按标签名查找子元素 / Find child elements by tag name
        // 注意：get_elements_by_tag_name 只查找直接子元素还是递归查找？
        // 根据 hps::Element 定义，get_elements_by_tag_name 返回匹配的子元素列表
        // Based on hps::Element definition, get_elements_by_tag_name returns a list of matched child elements

        // 我们来找一下 ul 下的 li
        // Let's find li under ul
        const auto uls = container->get_elements_by_tag_name("ul");
        if (!uls.empty()) {
            const auto* ul    = uls[0];
            const auto  items = ul->get_elements_by_tag_name("li");

            std::cout << "\nFound " << items.size() << " items:" << '\n';

            for (const auto* item : items) {
                // 5. 获取文本内容 / Get text content
                // text_content() 会递归获取所有子节点的文本
                // text_content() recursively gets text of all children
                std::cout << "- " << item->text_content();

                // 获取特定属性
                std::cout << " (ID: " << item->get_attribute("data-id") << ")" << '\n';
            }
        }
    } else {
        std::cout << "Container not found!" << '\n';
    }

    std::cout << "\n--------------------------------\n";

    // 6. 演示 querySelector (单个元素)
    // 6. Demonstrate querySelector (single element)
    const auto* link = doc->querySelector("a");
    if (link) {
        std::cout << "Link found:" << '\n';
        std::cout << "Href: " << link->get_attribute("href") << '\n';
        std::cout << "Text: " << link->text_content() << '\n';
    }

    system("pause");
    return 0;
}
