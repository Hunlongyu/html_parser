# Changelog

本项目的所有重要变更都会记录在此文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，
版本号遵循 [Semantic Versioning](https://semver.org/lang/zh-CN/)。

## [0.3.1] - 2026-06-08

进一步逼近 lexbor 的两项内部优化（非破坏）：tokenizer 的 SIMD 扫描与文本节点零拷贝。

### Performance

- **tokenizer memchr 扫描**：Data / Plaintext / RAWTEXT-RCDATA 状态原本逐字符找
  下一个 `<`，改用 `memchr`（CRT 的 SIMD 实现）批量跳读。文本密集页 tok_ms 提升明显
  （panlong −37%、shodan −15%、complex −12%），标签密集页温和（fofa/规范单页 −4%）。
- **文本节点零拷贝**：文本默认零拷贝持有指向源码/字符串池的 `string_view`，仅在相邻
  文本合并 append 时才物化为自有缓冲。无变换（Raw + 不解码 + Preserve）时直接零拷贝。
  Preserve 模式（保留文本的真实爬虫场景）下规范单页分配 −23%（34.7 万文本节点零拷贝）、
  fofa −11%。`performance()` 模式丢弃文本故不受影响。

### Notes

- 一致性保持 1227/1764；362/362 测试通过；AddressSanitizer 全程干净。
- 评估后**搁置**的项：tokenizer token/属性结构完整复用（需重写全部 consume_* 且破坏
  `tokenize_all` 拥有契约，ROI 低）；属性 small_vector（内联会膨胀节点、损缓存，且建树
  已是 CPU 受限，分配下降难转化为时间）。

[0.3.1]: https://github.com/Hunlongyu/html_parser/releases/tag/v0.3.1

## [0.3.0] - 2026-06-08

用现代 C++ 复刻 lexbor 的高性能 DOM 内存模型（在 0.2.0 节点 arena 之上）：
**名字整数化（tag-id / attr-id）+ 字符串 arena**。建树与 CSS 匹配的热路径以整数
比较取代字符串比较；节点名/属性/注释改为指向 Document 字符串池/源码的 string_view，
消除大量逐节点 std::string 分配。

### Changed (BREAKING)

- **访问器返回 string_view**：`Element::tag_name()`、`get_attribute()`、`id()`、
  `class_name()`、`Attribute::name()`/`value()`、`TextNode::value()`、
  `CommentNode::value()` 由 `const std::string&` 改为 `std::string_view`
  （指向 Document 拥有的源码/字符串池，与文档同生命周期）。多数用法（打印、比较、
  传 string_view）源码兼容；持久保存引用的调用方需调整。
- **Attribute 重构为轻量值类型**：持 `Attr` 整数 id + 名/值视图；移除 `TokenAttribute`
  构造与 `set_name`。
- 节点创建一律经 `Document::create_*` 工厂（驻留字符串 + 记录 owner_document）。

### Added

- **整数化标签/属性名**：`enum class Tag` / `Attr` + 内部 `tag_table.hpp` / `attr_table.hpp`
  （已知名排序主表、编译期标志表、命名常量；static_assert 保证完整性）。
  新增 `Element::tag()`、`Attribute::id()`。
- `Document::intern()`（源码子串零拷贝 / 否则入 StringPool）；`Node::owner_document`
  指针使 `owner_document()` 降为 O(1)（消除原父链上溯）。
- 性能语料：接入 WHATWG HTML 规范单页（事实标准大文件）作为基准输入。

### Fixed

- **tokenizer O(n²)（大文档）**：错误位置原本每次从源码起点扫描算行列；大页面产生
  O(n) 可恢复错误 → 退化为 O(n²)。改为增量行列计算（O(n)，行列结果不变）。
  WHATWG 规范单页（15.5MB）解析由 ~4 分钟降至 ~280ms。

### Performance

- 建树字符串比较整数化：tree-build 时间下降约 11%–34%。
- 名字/属性/注释池化：tree-build 分配下降约 24%–30%（a/node 由 ~2.85 降至 ~2.43）。
- 一致性保持 1227/1764；全程 AddressSanitizer 干净（含 15.5MB 规范单页全量回读 + 序列化）。

[0.3.0]: https://github.com/Hunlongyu/html_parser/releases/tag/v0.3.0

## [0.2.0] - 2026-06-06

DOM 节点所有权模型重构为 **arena 分配**：全部节点改由 `Document` 的单调
arena 持有（节点生命周期 = 文档生命周期），节点之间以裸指针互相引用，消除
逐节点 `malloc`。这是面向高性能爬取的标准 fast-DOM 架构（同 lexbor / 浏览器
/ 编译器 AST）。在真实体量页面上吞吐有可测提升，且不牺牲安全性（单一所有者）。

### Changed (BREAKING)

- **节点创建改用 Document 工厂**：新增 `Document::create_element` /
  `create_text` / `create_comment` / `create_doctype`，返回由文档 arena 拥有的
  裸指针。不再用 `std::make_unique<Element>(…)` 直接构造可挂树的节点。
- **子节点 API 改裸指针**：`add_child` / `insert_child_before` 现接收 `Node*`
  （不再转移 `std::unique_ptr` 所有权）；`remove_child` 返回 `void`；
  `take_children` 返回 `std::vector<Node*>`（仅断链，节点仍归 arena）。
- 子节点存储由 `std::vector<std::unique_ptr<Node>>` 改为侵入式链表
  （`m_first_child` / `m_last_child` + 既有兄弟链），父节点不再持有子节点容器。

### Added

- `NodeArena`：DOM 节点的单调 bump 分配器（块内分配、逆序虚析构、整体回收）。

### Performance

- 消除逐节点对象分配与 per-parent children 向量分配；在较大页面（数百至数千
  节点）上解析耗时下降约 1%–15%（小页面持平）。剩余分配主要来自各节点的
  `std::string` 成员（出于安全/易用性考量，本版本不做字符串驻留）。

[0.2.0]: https://github.com/Hunlongyu/html_parser/releases/tag/v0.2.0

## [0.1.0] - 2026-06-06

这是一个大版本跃迁：接入完整的 html5lib 一致性套件，并把 tree-construction
通过率从 **17.2% 提升到 69.6%**（1227 / 1764，与 parse5 / lexbor 同口径）。

### Added

- **html5lib 一致性套件**：接入完整的上游 `html5lib-tests` tree-construction
  套件（1764 例）作为报告型一致性测试，并提供 tokenizer 基线测试。
- **DOCTYPE 节点**：新增 `DoctypeNode`，保真解析 `<!DOCTYPE …>`（名称 +
  PUBLIC/SYSTEM 标识符）。
- **完整 HTML5 实体解码**：内置完整 WHATWG 具名字符引用表（约 2231 条，含旧式
  无分号形式），支持最长匹配、十进制/十六进制数值引用、C1→Windows-1252 重映射。
- **领养机构算法（Adoption Agency）+ 活动格式化元素列表**：正确处理 `<b><i>`
  类交叉误嵌套的格式化元素，含 marker、`<a>`/`<nobr>` 特例与 reconstruct。
- **外来内容（SVG / MathML）**：命名空间元素与子树构建、breakout 跳出、HTML
  集成点、SVG/MathML 属性大小写与命名空间修正、foster parenting。
- **`<template>`**：基础内容隔离（content fragment 表示）与表格内处理。
- **frameset 文档**：`<frameset>`/`<frame>`/`<noframes>` 的 in-frameset /
  after-frameset 处理。
- **script-data 状态机**：escaped / double-escaped 状态、带属性的结束标签。
- 文档外壳保真：非片段文档总是产出 `html`/`head`/`body`（frameset 文档除外）。
- `Node::remove_child` / `Element::remove_child`：支持树重排。

### Fixed

- 修复 tokenizer 在 `<div<div>`、自定义元素、畸形属性名、内嵌 NUL 等输入上的
  无进展死循环（DoS）。
- 修复领养机构算法内循环的栈记账错误（错删元素 / 提前退出）。
- 修复 `&nbsp;` 被错误解码为普通空格（应为 U+00A0）。
- 修复 `<svg>` 被误当作 RAWTEXT，导致内联 SVG 内容被吞为不透明文本。
- 表格上下文中的游离 `</body>`/`</html>` 不再拆毁 foster parenting 插入点。

### Changed

- CSS 选择器：去除冗余去重、收紧 id/属性匹配。
- 重构 `process_start_tag` 与 RAWTEXT/RCDATA 词法器（去重、可读性，无行为变化）。

[0.1.0]: https://github.com/Hunlongyu/html_parser/releases/tag/v0.1.0
