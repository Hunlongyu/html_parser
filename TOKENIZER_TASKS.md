# Tokenizer 优化与合规性任务清单

本文档列出了 `Tokenizer` 类中存在的标准合规性问题和性能优化机会。

## 1. 标准合规性修复 (Standard Compliance)

这些任务旨在确保解析器符合 HTML5 (WHATWG) 标准，保证解析结果的正确性。

- [ ] **修复 SVG 解析逻辑错误** (高优先级)
    - **问题**: 当前代码将 `<svg>` 标签强制切换为 `ScriptData` 状态（类似 `<script>`），导致 SVG 内部标签（如 `<path>`, `<rect>`）被视为纯文本无法解析。
    - **影响**: 无法构建 SVG DOM 树，SVG 内容显示错误。
    - **方案**: 移除 `<svg>` 的特殊状态切换逻辑，将其视为普通标签处理。

- [ ] **实现字符引用 (Entity) 解码** (高优先级)
    - **问题**: 在 `Data` 状态（普通文本）和属性值解析中，未处理 `&` 开头的字符实体（如 `&amp;`, `&#60;`）。
    - **影响**: 解析出的文本包含原始编码（如 `A&amp;B`）而非解码后的字符（`A&B`）。
    - **方案**: 引入实体解码器，在遇到 `&` 时尝试匹配命名实体或数字实体。

- [ ] **完善 DOCTYPE 解析** (中优先级)
    - **问题**: `consume_doctype_state` 仅匹配关键字后直接跳过，未提取 Name, Public ID, System ID。
    - **影响**: 无法正确检测 "Quirks Mode"（怪异模式），可能影响后续 CSS 渲染逻辑。
    - **方案**: 按照标准实现完整的 DOCTYPE 状态机，提取标识符并标记文档模式。

- [ ] **严谨的 RCDATA/RAWTEXT 结束检测** (低优先级)
    - **问题**: `ScriptData` 和 `RawText` 状态仅通过字符串前缀匹配（`starts_with("</script")`）来检测结束，未处理复杂的边界情况。
    - **影响**: 极端情况下（如脚本字符串中包含 `</script>`）可能导致解析提前结束。
    - **方案**: 实现标准要求的 "RCDATA less-than sign state" 等细粒度状态转换。

## 2. 性能优化 (Performance)

这些任务旨在降低内存占用并提高解析速度。

- [ ] **优化 Token 字符串存储 (std::string_view)** (高收益)
    - **问题**: `Token` 和 `TokenBuilder` 大量使用 `std::string`，导致频繁的内存分配和拷贝。
    - **影响**: 解析大型文档时内存开销大，速度受限于 `malloc`/`free`。
    - **方案**: 重构 `Token` 类，默认使用 `std::string_view` 指向源码 buffer。仅在需要转义或大小写转换时使用 `std::string` (CowString 模式)。

- [ ] **SIMD 加速字符扫描** (中收益)
    - **问题**: `consume_data_state` 使用逐字节循环查找 `<` 或 `&`。
    - **影响**: 在大段纯文本处理上 CPU 效率未达到极致。
    - **方案**: 使用 SSE/AVX 指令一次性检查 16/32 字节的数据块。

- [ ] **Token 对象池 (Object Pooling)** (中收益)
    - **问题**: `std::vector<Token>` 存储导致大量 Token 对象的构造和析构。
    - **影响**: 内存碎片化。
    - **方案**: 使用 Arena Allocator 或 Object Pool 批量管理 Token 内存。
