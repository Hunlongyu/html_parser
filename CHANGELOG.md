# Changelog

本项目的所有重要变更都会记录在此文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，
版本号遵循 [Semantic Versioning](https://semver.org/lang/zh-CN/)。

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
