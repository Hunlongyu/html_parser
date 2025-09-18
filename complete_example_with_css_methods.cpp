#include <hps/hps.hpp>
#include <hps/query/query.hpp>
#include <hps/query/element_query.hpp>
#include <iostream>
#include <vector>
#include <string>

using namespace hps;

// 示例HTML内容
const std::string sample_html = R"(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <title>CSS方法演示</title>
</head>
<body>
    <div class="container" id="main">
        <header class="site-header">
            <h1 class="title">网站标题</h1>
            <nav class="navigation">
                <ul class="nav-menu">
                    <li class="nav-item active"><a href="#home">首页</a></li>
                    <li class="nav-item"><a href="#about">关于</a></li>
                    <li class="nav-item"><a href="#contact">联系</a></li>
                </ul>
            </nav>
        </header>
        
        <main class="content">
            <section class="products">
                <h2>产品列表</h2>
                <div class="product-grid">
                    <div class="product-card" data-id="001">
                        <h3 class="product-name">产品A</h3>
                        <p class="product-price">¥199</p>
                        <div class="product-tags">
                            <span class="tag featured">推荐</span>
                            <span class="tag new">新品</span>
                        </div>
                    </div>
                    <div class="product-card" data-id="002">
                        <h3 class="product-name">产品B</h3>
                        <p class="product-price">¥299</p>
                        <div class="product-tags">
                            <span class="tag sale">促销</span>
                        </div>
                    </div>
                </div>
            </section>
            
            <aside class="sidebar">
                <div class="widget">
                    <h3>相关链接</h3>
                    <ul class="link-list">
                        <li><a href="https://example.com">外部链接1</a></li>
                        <li><a href="https://test.com">外部链接2</a></li>
                        <li><a href="/internal">内部链接</a></li>
                    </ul>
                </div>
            </aside>
        </main>
        
        <footer class="site-footer">
            <p>&copy; 2024 示例网站</p>
        </footer>
    </div>
</body>
</html>
)";

// 辅助函数：打印查询结果
void print_results(const ElementQuery& results, const std::string& description) {
    std::cout << "\n" << description << " (找到 " << results.size() << " 个元素):" << std::endl;
    
    for (size_t i = 0; i < results.size(); ++i) {
        auto element = results[i];
        if (element) {
            std::cout << "  [" << i + 1 << "] <" << element->tag_name();
            
            if (element->has_attribute("id")) {
                std::cout << " id=\"" << element->get_attribute("id") << "\"";
            }
            if (element->has_attribute("class")) {
                std::cout << " class=\"" << element->get_attribute("class") << "\"";
            }
            
            std::cout << ">";
            
            // 显示文本内容（限制长度）
            auto text = element->get_text_content();
            if (!text.empty()) {
                if (text.length() > 30) {
                    text = text.substr(0, 30) + "...";
                }
                std::cout << " 文本: \"" << text << "\"";
            }
            
            std::cout << std::endl;
        }
    }
}

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    std::cout << "HPS HTML解析库 - document.css() 和 element.css() 方法演示" << std::endl;
    std::cout << "版本: " << hps::version() << std::endl;
    
    try {
        // 解析HTML文档
        auto doc = hps::parse(sample_html);
        
        if (!doc) {
            std::cerr << "HTML解析失败!" << std::endl;
            return 1;
        }
        
        std::cout << "✓ HTML解析成功!" << std::endl;
        
        // ========================================
        // 1. Document.css() 方法演示
        // ========================================
        print_separator("1. Document.css() 方法演示");
        
        std::cout << "\ndocument.css() 方法可以直接在文档对象上调用CSS选择器查询" << std::endl;
        
        // 使用 document.css() 查询所有div元素
        auto all_divs = doc->css("div");
        print_results(all_divs, "doc->css(\"div\") - 所有div元素");
        
        // 使用 document.css() 查询所有产品卡片
        auto product_cards = doc->css(".product-card");
        print_results(product_cards, "doc->css(\".product-card\") - 所有产品卡片");
        
        // 使用 document.css() 查询主容器
        auto main_container = doc->css("#main");
        print_results(main_container, "doc->css(\"#main\") - 主容器");
        
        // 使用 document.css() 查询所有链接
        auto all_links = doc->css("a");
        print_results(all_links, "doc->css(\"a\") - 所有链接");
        
        // 使用 document.css() 查询属性选择器
        auto elements_with_data_id = doc->css("[data-id]");
        print_results(elements_with_data_id, "doc->css(\"[data-id]\") - 有data-id属性的元素");
        
        // 使用 document.css() 查询组合选择器
        auto nav_links = doc->css("nav a");
        print_results(nav_links, "doc->css(\"nav a\") - 导航中的链接");
        
        // 使用 document.css() 查询复合选择器
        auto featured_tags = doc->css("span.tag.featured");
        print_results(featured_tags, "doc->css(\"span.tag.featured\") - 推荐标签");
        
        // ========================================
        // 2. Element.css() 方法演示
        // ========================================
        print_separator("2. Element.css() 方法演示");
        
        std::cout << "\nelement.css() 方法可以在特定元素内进行CSS选择器查询" << std::endl;
        
        // 首先获取一个容器元素
        auto container = doc->css(".container").first_element();
        if (container) {
            std::cout << "\n找到容器元素: <" << container->tag_name() << " class=\"" 
                      << container->get_attribute("class") << "\">" << std::endl;
            
            // 在容器内查询所有段落
            auto container_paragraphs = container->css("p");
            print_results(container_paragraphs, "container->css(\"p\") - 容器内的所有段落");
            
            // 在容器内查询所有标题
            auto container_headings = container->css("h1, h2, h3");
            print_results(container_headings, "container->css(\"h1, h2, h3\") - 容器内的所有标题");
            
            // 在容器内查询特定类的元素
            auto container_tags = container->css(".tag");
            print_results(container_tags, "container->css(\".tag\") - 容器内的所有标签");
        }
        
        // 获取产品网格元素并在其内部查询
        auto product_grid = doc->css(".product-grid").first_element();
        if (product_grid) {
            std::cout << "\n找到产品网格元素: <" << product_grid->tag_name() << " class=\"" 
                      << product_grid->get_attribute("class") << "\">" << std::endl;
            
            // 在产品网格内查询产品名称
            auto product_names = product_grid->css(".product-name");
            print_results(product_names, "product_grid->css(\".product-name\") - 产品网格内的产品名称");
            
            // 在产品网格内查询价格
            auto product_prices = product_grid->css(".product-price");
            print_results(product_prices, "product_grid->css(\".product-price\") - 产品网格内的价格");
            
            // 在产品网格内查询所有span元素
            auto all_spans = product_grid->css("span");
            print_results(all_spans, "product_grid->css(\"span\") - 产品网格内的所有span");
        }
        
        // 获取导航元素并在其内部查询
        auto navigation = doc->css(".navigation").first_element();
        if (navigation) {
            std::cout << "\n找到导航元素: <" << navigation->tag_name() << " class=\"" 
                      << navigation->get_attribute("class") << "\">" << std::endl;
            
            // 在导航内查询所有列表项
            auto nav_items = navigation->css("li");
            print_results(nav_items, "navigation->css(\"li\") - 导航内的所有列表项");
            
            // 在导航内查询活跃项
            auto active_items = navigation->css(".active");
            print_results(active_items, "navigation->css(\".active\") - 导航内的活跃项");
            
            // 在导航内查询所有链接
            auto nav_links = navigation->css("a");
            print_results(nav_links, "navigation->css(\"a\") - 导航内的所有链接");
        }
        
        // ========================================
        // 3. 对比 Query::css() 静态方法
        // ========================================
        print_separator("3. 三种CSS查询方法对比");
        
        std::cout << "\n演示三种不同的CSS查询方法:" << std::endl;
        std::cout << "1. Query::css(document, selector) - 静态方法" << std::endl;
        std::cout << "2. document.css(selector) - 文档方法" << std::endl;
        std::cout << "3. element.css(selector) - 元素方法" << std::endl;
        
        // 同样的查询用三种方法实现
        std::string selector = ".product-name";
        
        // 方法1: Query::css 静态方法
        auto result1 = Query::css(*doc, selector);
        std::cout << "\nQuery::css(*doc, \"" << selector << "\") 找到: " << result1.size() << " 个元素" << std::endl;
        
        // 方法2: document.css 方法
        auto result2 = doc->css(selector);
        std::cout << "doc->css(\"" << selector << "\") 找到: " << result2.size() << " 个元素" << std::endl;
        
        // 方法3: 在特定元素内使用 element.css
        if (auto main_elem = doc->css("main").first_element()) {
            auto result3 = main_elem->css(selector);
            std::cout << "main_elem->css(\"" << selector << "\") 找到: " << result3.size() << " 个元素" << std::endl;
        }
        
        // ========================================
        // 4. 链式调用演示
        // ========================================
        print_separator("4. 链式调用演示");
        
        std::cout << "\n演示 document.css() 和 element.css() 的链式调用:" << std::endl;
        
        // document.css() 链式调用
        auto expensive_products = doc->css(".product-card")
                                      .has_class("product-card")
                                      .first(1);
        print_results(expensive_products, "doc->css(\".product-card\").has_class(\"product-card\").first(1)");
        
        // element.css() 链式调用
        if (auto sidebar = doc->css(".sidebar").first_element()) {
            auto sidebar_links = sidebar->css("a")
                                        .has_attribute("href")
                                        .containing_text("链接");
            print_results(sidebar_links, "sidebar->css(\"a\").has_attribute(\"href\").containing_text(\"链接\")");
        }
        
        // ========================================
        // 5. 数据提取演示
        // ========================================
        print_separator("5. 数据提取演示");
        
        // 使用 document.css() 提取数据
        std::cout << "\n使用 document.css() 提取数据:" << std::endl;
        
        auto product_names = doc->css(".product-name").extract_texts();
        std::cout << "\n所有产品名称:" << std::endl;
        for (size_t i = 0; i < product_names.size(); ++i) {
            std::cout << "  [" << i + 1 << "] " << product_names[i] << std::endl;
        }
        
        auto product_prices = doc->css(".product-price").extract_texts();
        std::cout << "\n所有产品价格:" << std::endl;
        for (size_t i = 0; i < product_prices.size(); ++i) {
            std::cout << "  [" << i + 1 << "] " << product_prices[i] << std::endl;
        }
        
        auto all_hrefs = doc->css("a").extract_attributes("href");
        std::cout << "\n所有链接地址:" << std::endl;
        for (size_t i = 0; i < all_hrefs.size(); ++i) {
            std::cout << "  [" << i + 1 << "] " << all_hrefs[i] << std::endl;
        }
        
        // 使用 element.css() 提取特定区域的数据
        std::cout << "\n使用 element.css() 提取特定区域的数据:" << std::endl;
        
        if (auto products_section = doc->css(".products").first_element()) {
            auto section_headings = products_section->css("h2, h3").extract_texts();
            std::cout << "\n产品区域的标题:" << std::endl;
            for (size_t i = 0; i < section_headings.size(); ++i) {
                std::cout << "  [" << i + 1 << "] " << section_headings[i] << std::endl;
            }
        }
        
        if (auto sidebar = doc->css(".sidebar").first_element()) {
            auto sidebar_links = sidebar->css("a").extract_attributes("href");
            std::cout << "\n侧边栏的链接:" << std::endl;
            for (size_t i = 0; i < sidebar_links.size(); ++i) {
                std::cout << "  [" << i + 1 << "] " << sidebar_links[i] << std::endl;
            }
        }
        
        // ========================================
        // 6. 实际应用场景
        // ========================================
        print_separator("6. 实际应用场景");
        
        std::cout << "\n实际应用场景演示:" << std::endl;
        
        // 场景1: 提取页面导航结构
        std::cout << "\n=== 场景1: 提取页面导航结构 ===" << std::endl;
        if (auto nav = doc->css("nav").first_element()) {
            auto nav_items = nav->css("a");
            std::cout << "导航菜单项:" << std::endl;
            nav_items.each([](size_t index, const Element& link) {
                std::cout << "  " << (index + 1) << ". " << link.get_text_content() 
                          << " -> " << link.get_attribute("href") << std::endl;
            });
        }
        
        // 场景2: 提取产品信息
        std::cout << "\n=== 场景2: 提取产品信息 ===" << std::endl;
        auto products = doc->css(".product-card");
        products.each([](size_t index, const Element& product) {
            std::cout << "产品 " << (index + 1) << ":" << std::endl;
            
            // 在每个产品元素内查询具体信息
            if (auto name = product.css(".product-name").first_element()) {
                std::cout << "  名称: " << name->get_text_content() << std::endl;
            }
            
            if (auto price = product.css(".product-price").first_element()) {
                std::cout << "  价格: " << price->get_text_content() << std::endl;
            }
            
            if (product.has_attribute("data-id")) {
                std::cout << "  ID: " << product.get_attribute("data-id") << std::endl;
            }
            
            // 提取标签
            auto tags = product.css(".tag").extract_texts();
            if (!tags.empty()) {
                std::cout << "  标签: ";
                for (size_t i = 0; i < tags.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << tags[i];
                }
                std::cout << std::endl;
            }
            
            std::cout << std::endl;
        });
        
        // 场景3: 分析页面结构
        std::cout << "\n=== 场景3: 分析页面结构 ===" << std::endl;
        
        // 统计各种元素数量
        std::cout << "页面结构统计:" << std::endl;
        std::cout << "  总div数量: " << doc->css("div").size() << std::endl;
        std::cout << "  总链接数量: " << doc->css("a").size() << std::endl;
        std::cout << "  标题数量 (h1-h6): " << doc->css("h1,h2,h3,h4,h5,h6").size() << std::endl;
        std::cout << "  段落数量: " << doc->css("p").size() << std::endl;
        std::cout << "  列表项数量: " << doc->css("li").size() << std::endl;
        
        // 分析特定区域
        if (auto main_content = doc->css("main").first_element()) {
            std::cout << "\n主内容区域分析:" << std::endl;
            std::cout << "  section数量: " << main_content->css("section").size() << std::endl;
            std::cout << "  产品卡片数量: " << main_content->css(".product-card").size() << std::endl;
            std::cout << "  标签数量: " << main_content->css(".tag").size() << std::endl;
        }
        
        print_separator("演示完成");
        std::cout << "\n✓ document.css() 和 element.css() 方法演示完成!" << std::endl;
        
        std::cout << "\n主要区别总结:" << std::endl;
        std::cout << "• Query::css(doc, selector) - 静态方法，需要传入文档参数" << std::endl;
        std::cout << "• doc->css(selector) - 文档实例方法，在整个文档中查询" << std::endl;
        std::cout << "• element->css(selector) - 元素实例方法，在特定元素内查询" << std::endl;
        std::cout << "• 所有方法都返回 ElementQuery 对象，支持链式调用" << std::endl;
        std::cout << "• element.css() 特别适合在特定容器内进行局部查询" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n错误: " << e.what() << std::endl;
        return 1;
    }
    
#ifdef _WIN32
    system("pause");
#endif
    
    return 0;
}