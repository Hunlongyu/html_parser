## 问题综述
- Document 的查询方法基本是对 CSS 查询的薄包装，内部逻辑重复但可维持；可通过私有辅助统一。
- 部分行为与 HTML 语义存在偏差或鲁棒性不足：root 不优先返回 <html>；charset 解析对 http-equiv 与 "charset=" 的大小写不够鲁棒；meta name/property 比较应更宽容。
- 查询链路在 ElementQuery::css 采用按指针排序 + unique，会破坏遍历顺序且无语义；兄弟导航部分未充分利用 Node 的兄弟指针，存在线性扫描开销。
- 多处可用 C++20 的 string_view、ranges、[[nodiscard]]/noexcept 等提升性能与可用性。

## 计划改动
### Document 语义与鲁棒性
1. root() 先返回 html()，若不存在再退回第一个元素子节点。
2. charset()：
   - http-equiv 比较改为大小写不敏感；
   - content 中查找 "charset=" 采用大小写不敏感匹配与去空白；
   - 返回值缓存维持不变。
3. get_meta_content/property：对目标值采用大小写不敏感比较，提升兼容性。
4. source_html() 返回 std::string_view 并标注 noexcept，避免不必要拷贝。

### 查询与集合行为（影响 Document 的所有查询结果）
5. ElementQuery::css 改为保序去重：使用 unordered_set 记录 seen，按插入顺序收集，移除当前无语义的指针排序。
6. siblings/next_sibling/prev_sibling/next_siblings/prev_siblings 改为使用 Node 的 m_prev_sibling/m_next_sibling 指针链，过滤非元素节点，消除线性扫描。
7. children(selector) 在遍历子节点时直接匹配并收集，避免先构建完整 children 集合再过滤。
8. 为 Query 增加接收预解析 SelectorList 的重载，Document 内部高频查询时可重用解析结果。

### C++20 接口与实现优化
9. 为轻量 getter 与纯集合方法补充 [[nodiscard]] 与 noexcept，增加 const 迭代器（ElementQuery）。
10. 在若干过滤/映射处使用 std::ranges 组合，减少中间容器；保持外部 API 不变。

### 测试与验证
11. 为 Document::root/html、charset 解析与 ElementQuery 的顺序与去重行为补充单元测试，确保改动符合预期。
12. 回归现有选择器与聚合（extract_attributes/extract_texts）的断言，更新可能依赖旧排序的测试。

## 交付方式
- 逐文件提交改动（document.hpp/cpp、element_query.cpp、query.cpp），每步附带小型基准或单元测试。
- 保持外部 API 二进制兼容；只有 source_html 的返回类型从 std::string 变为 std::string_view，若需保持签名，可先增设重载，再在次版本移除旧接口。

请确认以上改动方向；确认后我将按上述步骤提交具体代码修改与测试。