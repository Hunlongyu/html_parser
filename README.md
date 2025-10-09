# HPS HTML 解析库

[![GitHub release](https://img.shields.io/github/v/release/Hunlongyu/html_parser?display_name=tag)](https://github.com/Hunlongyu/html_parser/releases/latest)
[![C++](https://img.shields.io/badge/C++-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

> ⚠️ **开发中 (WIP)** - 这是一个正在积极开发的项目，API 可能会发生变化。不建议在生产环境中使用当前版本。

一个高性能、现代化的 C++ HTML 解析库，支持完整的 HTML5 标准解析和 CSS 选择器查询。

## ✨ 核心特性

### 🔍 强大的 CSS 选择器支持
- **复杂选择器**：支持后代选择器、子选择器、相邻兄弟选择器、通用兄弟选择器
- **属性选择器**：支持 `[attr]`、`[attr=value]`、`[attr^=value]`、`[attr$=value]`、`[attr*=value]` 等
- **伪类选择器**：支持 `:first-child`、`:last-child`、`:nth-child()`、`:not()` 等
- **组合选择器**：支持多重选择器组合和复杂嵌套查询

### ⛓️ 流畅的链式查询 API
- **链式调用**：`doc->css(".container").find("h1").has_class("title").first_element()`
- **过滤方法**：`has_class()`、`has_attribute()`、`containing_text()`、`matches()` 等
- **导航方法**：`parent()`、`children()`、`siblings()`、`closest()`、`find()` 等
- **结果处理**：`first_element()`、`last_element()`、`at(index)`、`filter()` 等

### ⚙️ 灵活的解析配置项
- **解析选项**：自定义解析行为，控制容错级别和处理策略
- **片段解析**：支持 HTML 片段解析，无需完整文档结构
- **错误处理**：可配置的错误恢复策略和警告级别

### 📄 双层查询架构
- **Document 查询**：从文档根节点开始的全局查询
  ```cpp
  auto results = doc->css("div.container p");
  ```
- **Element 查询**：从任意元素节点开始的局部查询
  ```cpp
  auto container = doc->css(".container").first_element();
  auto paragraphs = container->css("p");  // 仅在容器内查询
  ```

### 🚀 高性能解析引擎
- **标准兼容**：完全遵循 HTML5 解析规范（Tokenizer → Tree Construction → DOM Tree）
- **内存优化**：内存池管理、零拷贝设计、智能缓存机制
- **查询加速**：ID/类名索引、LRU 策略优化
- **现代 C++23**：模块化架构，充分利用新语言特性
- **无第三方依赖**：仅依赖 C++23 标准库，无需额外安装任何第三方库

## 🚀 快速演示

### 基础查询示例
```cpp
#include <hps/hps.hpp>
#include <iostream>

int main() {
    std::string html = R"(
        <div class="container">
            <header class="site-header">
                <h1 id="title" class="main-title">网站标题</h1>
                <nav class="navigation">
                    <ul class="nav-menu">
                        <li class="nav-item active"><a href="#home">首页</a></li>
                        <li class="nav-item"><a href="#about">关于</a></li>
                    </ul>
                </nav>
            </header>
            <main class="content">
                <article class="post" data-id="123">
                    <h2 class="post-title">文章标题</h2>
                    <p class="post-content">文章内容...</p>
                </article>
            </main>
        </div>
    )";
    
    // 解析配置
    hps::Options options;
    options.encoding = "UTF-8";
    options.fragment_parsing = true;
    
    auto doc = hps::parse(html, options);
    
    // 复杂 CSS 选择器查询
    auto activeNav = doc->css(".navigation .nav-item.active a").first_element();
    std::cout << "活跃导航: " << activeNav->text_content() << std::endl;
    
    // 属性选择器
    auto article = doc->css("article[data-id='123']").first_element();
    std::cout << "文章ID: " << article->get_attribute("data-id") << std::endl;
    
    return 0;
}
```

### 链式查询示例
```cpp
#include <hps/hps.hpp>

int main() {
    std::string html = R"(
        <div class="products">
            <div class="product featured" data-price="199">
                <h3 class="name">产品A</h3>
                <span class="price">¥199</span>
                <div class="tags">
                    <span class="tag new">新品</span>
                    <span class="tag sale">促销</span>
                </div>
            </div>
            <div class="product" data-price="299">
                <h3 class="name">产品B</h3>
                <span class="price">¥299</span>
            </div>
        </div>
    )";
    
    auto doc = hps::parse(html);
    
    // 链式查询：查找推荐产品中的促销商品
    auto featuredSaleProducts = doc->css(".product")
                                  .has_class("featured")
                                  .find(".tag")
                                  .containing_text("促销")
                                  .closest(".product");
    
    for (const auto& product : featuredSaleProducts) {
        auto name = product->css(".name").first_element()->text_content();
        auto price = product->get_attribute("data-price");
        std::cout << "推荐促销产品: " << name << ", 价格: ¥" << price << std::endl;
    }
    
    // Element 级别查询
    auto productsContainer = doc->css(".products").first_element();
    auto cheapProducts = productsContainer->css(".product")
                           .filter([](const auto& elem) {
                               auto price = std::stoi(elem->get_attribute("data-price"));
                               return price < 250;
                           });
    
    std::cout << "找到 " << cheapProducts.size() << " 个便宜产品" << std::endl;
    
    return 0;
}
```

## 📦 快速安装

### 系统要求
- C++20 兼容编译器（MSVC 2022+）
- CMake 3.28+

### 构建步骤
```bash
git clone https://github.com/Hunlongyu/html_parser.git
cd html_parser
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### CMake 集成
```cmake
add_subdirectory(path/to/hps)
target_link_libraries(your_target hps_static)
```

## 📚 文档链接

- [📖 详细 API 文档](docs/API.md)
- [🔧 详细构建说明](docs/BUILD.md)
- [💡 完整示例代码](examples/)
- [🏗️ 设计文档](docs/HPS%20HTML%20解析库详细设计文档.md)

## 📋 TODO

- [ ] **XPath 支持**：完整的 XPath 1.0 表达式支持（开发中）
- [ ] 性能基准测试

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

- 🐛 发现 Bug？[提交 Issue](https://github.com/Hunlongyu/html_parser/issues)
- 💡 有新想法？[讨论功能请求](https://github.com/Hunlongyu/html_parser/discussions)
- 🔧 想要贡献代码？查看 [贡献指南](CONTRIBUTING.md)

## 📄 许可证
本项目采用 [MIT 许可证](LICENSE)。

---

⭐ 如果这个项目对您有帮助，请给我们一个 Star！