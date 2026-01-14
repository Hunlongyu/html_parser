# 优化与完善 Element 类

经过分析，`Element` 类存在一些逻辑遗漏、性能瓶颈以及冗余操作。以下是详细的优化计划：

## 1. 逻辑修正 (Bug Fixes)

### 递归查询问题

目前 `get_elements_by_tag_name` 和 `get_elements_by_class_name` 仅遍历 **直接子节点**。

* **问题**：根据 DOM 标准（及 `get_element_by_id` 的实现），这些方法应当是 **递归** 的（搜索整个子树）。

* **修正**：修改这两个函数的实现，使其递归搜索所有后代节点。

## 2. 性能优化 (Performance)

### `get_attribute` 返回值优化

* **现状**：`std::string get_attribute(...)` 按值返回，导致不必要的字符串拷贝。

* **优化**：改为 `const std::string& get_attribute(...)`。当属性不存在时，返回一个静态的空字符串引用。

### `has_class` 内存分配优化

* **现状**：`has_class` 调用 `split_class_names`，这会创建一个 `std::unordered_set`，导致堆内存分配，仅仅是为了检查一个是否存在。

* **优化**：直接在 `class` 属性字符串上进行遍历匹配（需处理边界情况，如 "class-name" 不应匹配 "class"），避免任何内存分配。

### `class_names` 拷贝优化

* **现状**：返回 `std::unordered_set<std::string>`，涉及大量字符串拷贝。

* **优化**：虽然返回集合本身需要拷贝，但可以考虑缓存机制（由于增加复杂度，暂不实施，优先解决 `has_class` 的热路径问题）。

  <br />

## 执行步骤

1. **修改头文件 (`include/hps/core/element.hpp`)**：

   * 更新 `get_attribute` 返回类型。

   * 添加 `remove_attribute` 声明。
2. **修改源文件 (`src/core/element.cpp`)**：

   * 实现递归版本的 `get_elements_by_tag_name` 和 `get_elements_by_class_name`。

   * 优化 `get_attribute` 实现。

   * 优化 `has_class` 实现（无分配算法）。

确认后将开始执行代码修改。
