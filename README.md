# HPS HTML 解析库

![GitHub Release](https://img.shields.io/github/v/release/Hunlongyu/html_parser)
![C++](https://img.shields.io/badge/C++-20-blue.svg)
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
- **过滤方法**：`has_class()`、`has_attribute()`、`containing_text()`、`is()`、`not_()` 等
- **导航方法**：`parent()`、`children()`、`siblings()`、`closest()`、`find()` 等
- **结果处理**：`first_element()`、`last_element()`、`at(index)`、`filter()` 等
- **数据提取（cheerio 风格）**：`text()`、`attr(name)`、`html()`，以及向量版 `extract_texts()`、`extract_attributes()`、`extract_outer_html()`

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
- **现代 C++20**：目标导向的库结构，便于集成、安装与导出
- **零运行时第三方依赖**：库本体仅依赖 C++20 标准库，测试使用 GoogleTest

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
    
    // html 已是 UTF-8 文本，直接解析即可
    auto doc = hps::parse(html);
    
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

### 🌐 编码处理（爬虫友好）

`parse` / `parse_fragment` 默认输入已是 **UTF-8**。当你拿到的是**未知编码的原始字节**（HTTP 响应体、本地文件）时，先用编码模块检测/转码——转码使用**系统原生 codec**（Windows: `MultiByteToWideChar`；POSIX: `iconv`），**不引入 ICU 等第三方库**。

#### 一步到位：检测 + 转码 + 解析

```cpp
// 便捷入口：内部完成 检测→转码→解析（最常用）
auto doc  = hps::parse_bytes(response_body);     // 原始字节，自动判定编码
auto doc2 = hps::parse_file("page.html");        // 文件同样自动转码

// 已知编码（如来自 HTTP 头 Content-Type: charset=gbk）时显式指定，跳过嗅探
hps::Options opts;
opts.encoding = "gbk";                            // 大小写、别名（gb2312/gb18030/...）均可
auto doc3 = hps::parse_bytes(response_body, opts);
```

#### 显式流程：完全掌控（推荐用于爬虫）

`decode_to_utf8` 返回精确的状态与实际使用的编码，便于日志记录与自定义回退：

```cpp
#include <hps/utils/encoding.hpp>

const auto r = hps::decode_to_utf8(response_body);   // 自动检测；或传第二参数强制编码
switch (r.status) {
    case hps::DecodeStatus::Ok:
        // r.text 是 UTF-8；r.encoding 是实际编码；r.source 说明判定来源
        { auto doc = hps::parse(r.text); /* ... */ }
        break;
    case hps::DecodeStatus::UnknownEncoding:
        // 无 BOM/声明且非合法 UTF-8：可自行回退（如按 windows-1252）或借助第三方库
        break;
    case hps::DecodeStatus::UnsupportedEncoding:  // 判定出但本库不支持（如 euc-kr）
    case hps::DecodeStatus::InvalidBytes:         // 字节与编码不符
        break;
}

// 只检测、不转码：
const auto d = hps::detect_encoding(response_body);
if (d.found()) { /* d.label / d.source / d.supported / d.bom_length */ }

// 能力查询：
bool ok = hps::is_encoding_supported("GB2312");      // true
for (auto enc : hps::supported_encodings()) { /* ... */ }
```

**已支持转码的编码**：UTF-8 / UTF-16 (LE·BE) / GBK·GB2312·GB18030 / Big5 / Shift_JIS / windows-1252·ISO-8859-1。
便捷入口（`parse_bytes` / `parse_file`）遇到无法识别或不支持的编码时记录 `UnsupportedEncoding`（严格模式抛异常）；需要按状态分支处理时改用上面的 `decode_to_utf8`，自行转成 UTF-8 后再交给 `parse`。

### 📤 数据提取（cheerio 风格）

```cpp
auto doc = hps::parse(html);

// 标量便捷取值：取首个匹配
std::string title = doc->css("h1.title").text();          // 合并文本（递归）
if (auto href = doc->css("a.next").attr("href")) {        // optional：无匹配/无该属性 → 空
    download(std::string(*href));
}

// Element 级：attr() 可区分“属性不存在”与“值为空”
const auto* img = doc->query_selector("img");
if (auto alt = img->attr("alt")) { /* 存在（可能为空串）*/ }  // get_attribute 无法区分二者

// 向量版：批量抓取
std::vector<std::string> links = doc->css("a").extract_attributes("href");
std::vector<std::string> rows  = doc->css("td").extract_texts();
```

### 🔗 URL 解析（相对→绝对）

页面里的 `href`/`src` 多是相对地址，爬虫几乎都需要解析为绝对地址。通过 `Options::base_url` 提供页面 URL 后，`get_all_links()` / `get_all_images()` 会**自动解析为绝对地址**（并自动考虑文档内的 `<base href>`）：

```cpp
hps::Options opts;
opts.base_url = "https://example.com/blog/post.html";   // 页面地址（通常来自抓取时的 URL）
auto doc = hps::parse_bytes(response_body, opts);

doc->get_all_links();            // 已解析为绝对 URL（未设 base_url 时保持原始值）
doc->base_url();                 // 有效基准（若有 <base href> 则已并入）
doc->resolve_url("../a/b.html"); // 解析任意单个引用
```

也可直接使用底层函数（RFC 3986 §5 参考解析，等价于 Python `urljoin` / JS `new URL(ref, base)`）：

```cpp
#include <hps/utils/url.hpp>

hps::resolve_url("https://example.com/a/b/page.html", "../img/x.png");
// → "https://example.com/a/img/x.png"
hps::resolve_url("https://site/p", "//cdn/lib.js");   // → "https://cdn/lib.js"（协议相对）
hps::is_absolute_url("mailto:a@b");                   // → true
```

`mailto:`/`tel:`/`data:` 等带 scheme 的引用按绝对地址原样返回；`base_url` 为空时 `resolve_url` 原样返回引用。

### 🧩 HTML 序列化（提取原始标记）

爬虫常需要拿到某节点的**原始 HTML 片段**（存档、二次解析、提取富文本）。`inner_html()` / `outer_html()` 可将 DOM 子树序列化回 HTML：

```cpp
auto doc = hps::parse(R"(<div id="a" class="x"><p>Hi</p></div>)");
const auto* div = doc->query_selector("div");

div->outer_html();   // <div id="a" class="x"><p>Hi</p></div>（含自身标签）
div->inner_html();   // <p>Hi</p>（仅子节点）

doc->css("li").html();              // 首个匹配元素的 inner_html（cheerio 风格）
doc->css("li").extract_outer_html();// 各匹配元素的 outer_html 向量
doc->outer_html();                  // 整个文档树序列化（反映解析后的 DOM）
```

序列化特性：void 元素（`<br>`/`<img>`…）不输出闭合标签；raw text 元素（`<script>`/`<style>`）内容原样输出；文本/属性做**实体感知转义**——源中已有的 `&amp;`/`&lt;` 不会被二次转义，默认（未解码实体）解析模式下可忠实还原源码片段。

## 📦 快速安装

### 系统要求
- C++20 兼容编译器（MSVC 2022+）
- CMake 3.28+

### 推荐构建入口
```bash
git clone https://github.com/Hunlongyu/html_parser.git
cd html_parser
cmake --preset dev-debug
cmake --build --preset dev-debug
ctest --preset dev-debug
```

### 推荐目录布局
- `include/hps/`：对外公开头文件
- `src/`：库实现与库目标定义
- `tests/`：单元测试与基线测试
- `examples/`：示例程序与示例资源
- `benchmark/`：性能基准
- `cmake/`：安装导出、编译选项和工具链辅助模块

### 常用 Presets
- `dev-debug`：本地开发默认配置，包含 tests 和 examples
- `dev-release`：发布前的 Release 构建
- `tidy`：打开 clang-tidy 的调试构建
- `package`：用于安装/导出的精简 Release 构建

### CMake 集成
```cmake
add_subdirectory(path/to/hps)
target_link_libraries(your_target PRIVATE hps::hps)
```

```cmake
find_package(hps CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE hps::hps)
```

### 开发工具配置
- `.clang-format`：统一 C++ 源码格式
- `.clang-tidy`：默认开启 correctness、modernize 和 performance 检查
- `.clangd`：为 clangd 提供一致的编辑器体验
- `.editorconfig`：约束缩进、换行和尾随空白
- `.cmake-format.yaml`：统一 CMake 脚本格式
- `CMakePresets.json`：统一 configure/build/test 工作流

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