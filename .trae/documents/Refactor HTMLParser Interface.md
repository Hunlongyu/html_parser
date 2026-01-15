# HTMLParser 接口精简计划

您对 `parse` 接口冗余的观察非常准确。针对 C++17/20 标准，我们可以大幅精简接口，同时保留性能优化。

## 现状分析

目前 `HTMLParser` 提供了 6 个 `parse` 重载和 2 个 `parse_file` 重载，主要冗余在于：
1.  **`const char*` 重载**：在 C++17 中，`const char*` 可以隐式转换为 `std::string_view`，且性能损耗极低，因此显式的 C 字符串重载是多余的。
2.  **`Options` 重载**：可以通过默认参数 `const Options& options = {}` 来合并。

保留 `std::string&&` 重载是**必要**的，因为它允许将临时字符串的所有权直接转移给 `Document`，避免由于拷贝整个 HTML 内容产生的巨大开销。

## 改造方案

我们将把接口从 **8 个减少到 3 个**（核心解析 2 个 + 文件解析 1 个）。

### 1. 修改头文件 `include/hps/parsing/html_parser.hpp`

我们将使用默认参数合并接口，并移除 `const char*` 版本。

**保留并修改：**
```cpp
// 1. 通用接口（支持 string_view, const char*, const string&）
[[nodiscard]] std::shared_ptr<Document> parse(std::string_view html, const Options& options = {});

// 2. 移动语义优化接口（支持 string&& 临时对象）
[[nodiscard]] std::shared_ptr<Document> parse(std::string&& html, const Options& options = {});

// 3. 文件解析接口
[[nodiscard]] std::shared_ptr<Document> parse_file(std::string_view filePath, const Options& options = {});
```

**删除：**
- `parse(const char*)` 及其 Options 版本
- 不带 Options 的 `parse(string_view)` 和 `parse(string&&)`（已被默认参数覆盖）

### 2. 修改源文件 `src/parsing/html_parser.cpp`

- 更新实现以匹配新的函数签名。
- 移除被删除接口的实现代码。
- 确保 `Options` 的默认构造函数可用（通常已包含头文件）。

## 预期效果

- **API 更简洁**：用户调用方式不变，但头文件更清爽。
- **性能不变**：
    - `parse("<html>")` -> 自动转换为 `string_view`，无额外开销。
    - `parse(std::move(str))` -> 命中 `string&&` 重载，零拷贝。
- **兼容性**：现有的调用代码（包括传 `const char*`）无需修改即可编译通过。

我们将立即执行此重构。
