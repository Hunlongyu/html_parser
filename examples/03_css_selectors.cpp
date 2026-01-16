/**
 * @file 03_css_selectors.cpp
 * @brief CSS 选择器使用示例 / CSS Selectors Example
 *
 * 本示例展示了如何使用 CSS 选择器查找元素：
 * 1. 基础选择器 (ID, Class, Tag)
 * 2. 组合选择器 (Descendant, Child, Adjacent Sibling, General Sibling)
 * 3. 属性选择器
 * 4. 伪类选择器 (first-child, last-child, nth-child)
 *
 * This example demonstrates how to find elements using CSS selectors:
 * 1. Basic selectors (ID, Class, Tag)
 * 2. Combinators (Descendant, Child, Adjacent Sibling, General Sibling)
 * 3. Attribute selectors
 * 4. Pseudo-class selectors (first-child, last-child, nth-child)
 */

#include "hps/core/element.hpp"
#include "hps/hps.hpp"

#include <iostream>
#include <vector>

void print_elements(const std::string& description, const std::vector<const hps::Element*>& elements) {
    std::cout << ">>> " << description << " (Found: " << elements.size() << ")" << std::endl;
    for (const auto* el : elements) {
        std::cout << "    <" << el->tag_name();
        if (el->has_attribute("id"))
            std::cout << " id=\"" << el->id() << "\"";
        if (el->has_attribute("class"))
            std::cout << " class=\"" << el->class_name() << "\"";
        std::cout << "> " << el->text_content() << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    std::string html = R"(
        <div id="app">
            <header>
                <nav>
                    <ul class="menu">
                        <li class="active"><a href="/">Home</a></li>
                        <li><a href="/products">Products</a></li>
                        <li><a href="/about">About</a></li>
                    </ul>
                </nav>
            </header>
            <main>
                <section class="content">
                    <h1>Welcome</h1>
                    <p class="intro">This is an introduction.</p>
                    <p>Another paragraph.</p>
                    <div class="gallery">
                        <img src="1.jpg" alt="Image 1" data-category="nature">
                        <img src="2.jpg" alt="Image 2" data-category="city">
                        <img src="3.jpg" alt="Image 3" data-category="nature">
                    </div>
                    <form>
                        <input type="text" name="username" required>
                        <input type="password" name="password">
                        <input type="submit" value="Login">
                    </form>
                </section>
                <aside>
                    <div class="widget">Widget 1</div>
                    <div class="widget">Widget 2</div>
                </aside>
            </main>
        </div>
    )";

    auto doc = hps::parse(html);

    // 1. 基础选择器 / Basic Selectors
    print_elements("Tag Selector (p)", doc->querySelectorAll("p"));
    print_elements("Class Selector (.menu)", doc->querySelectorAll(".menu"));
    print_elements("ID Selector (#app)", doc->querySelectorAll("#app"));

    // 2. 层级组合选择器 / Hierarchy Combinators
    // 后代选择器 (Descendant): div a
    print_elements("Descendant Selector (div a)", doc->querySelectorAll("div a"));

    // 子元素选择器 (Child): ul > li
    print_elements("Child Selector (ul > li)", doc->querySelectorAll("ul > li"));

    // 3. 属性选择器 / Attribute Selectors
    // 存在属性
    print_elements("Attribute Exists ([required])", doc->querySelectorAll("[required]"));

    // 属性值匹配
    print_elements("Attribute Value ([type=\"submit\"])", doc->querySelectorAll("[type=\"submit\"]"));

    // 4. 伪类选择器 / Pseudo-class Selectors
    // :first-child
    print_elements("First Child (li:first-child)", doc->querySelectorAll("li:first-child"));

    // :nth-child
    // 注意：具体支持的伪类取决于 hps 的实现
    // Note: Supported pseudo-classes depend on hps implementation
    print_elements("Nth Child (li:nth-child(2))", doc->querySelectorAll("li:nth-child(2)"));

    // 5. 链式查询 (Query Object) / Chained Query
    // 使用 .css() 方法获取 ElementQuery 对象
    // Use .css() method to get ElementQuery object
    std::cout << ">>> Chained Query (.menu -> li -> a)" << std::endl;
    auto links = doc->css(".menu").css("li").css("a");
    for (const auto* link : links) {
        std::cout << "    Link: " << link->get_attribute("href") << " - " << link->text_content() << std::endl;
    }

    system("pause");
    return 0;
}
