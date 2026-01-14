# 移除 XPath 代码和文档的计划

我将执行以下步骤来移除所有 XPath 相关的代码和文档：

## 1. 删除源文件和头文件
将删除以下目录和文件：
- `include/hps/query/xpath/` (整个目录)
- `src/query/xpath/` (整个目录)
- `examples/xpath_parser.cpp`
- `examples/xpath_lexer.cpp`
- `examples/xpath_ast.cpp`

## 2. 修改代码移除引用
将修改以下文件以移除对 XPath 的引用和接口：

### 头文件
- **`include/hps/query/element_query.hpp`**: 移除 `xpath()` 方法声明。
- **`include/hps/query/query.hpp`**: 移除 `xpath()` 静态方法声明。
- **`include/hps/core/document.hpp`**: 移除 `xpath()` 方法声明。
- **`include/hps/core/element.hpp`**: 移除 `xpath()` 方法声明。

### 源文件
- **`src/query/element_query.cpp`**: 移除 `xpath()` 方法实现。
- **`src/query/query.cpp`**: 移除 `xpath()` 方法实现。
- **`src/core/document.cpp`**: 移除 `xpath()` 方法实现。
- **`src/core/element.cpp`**: 移除 `xpath()` 方法实现。

### 构建文件
- **`CMakeLists.txt`**: 移除已注释的 XPath 源文件引用。
- **`examples/CMakeLists.txt`**: 移除 `xpath_ast`, `xpath_lexer`, `xpath_parser` 示例目标。

## 3. 验证
- 运行 CMake 配置以确保构建系统正常。
- 编译项目以验证没有遗留的断裂引用。
