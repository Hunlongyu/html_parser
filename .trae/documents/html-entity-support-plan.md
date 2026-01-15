## 目标与原则
- **优先 DOM 结构解析**：不把字符引用当成标签语法的一部分处理（即不会把 `&lt;div&gt;` 变成元素）。
- **默认不做“文本美化”**：用户拿到 TextNode/Attribute value 后可自行处理。
- **仅提供最小、可选的解码能力**：不引入 HTML5 2520+ 命名实体表。

## 需要支持/不支持的实体范围
- **保留（最小命名实体集合）**：`&amp; &lt; &gt; &quot; &apos;`（可选保留 `&nbsp;`，因为已存在且高频）。
- **增加（严格数字实体）**：
  - 十进制：`&#65;` → `A`
  - 十六进制：`&#x41;` / `&#X41;` → `A`
  - **严格模式**：必须以 `;` 结尾；超范围/非法数字不解码（原样保留），避免误伤。
  - **“极小部分”的实现方式**：不做 legacy 兼容（如缺分号、命名实体无分号），并加长度上限（如十进制最多 8 位、十六进制最多 6 位），防止恶意超长输入拖慢解析。
- **不实现**：`&copy; &reg; &euro; &alpha; ...` 等所有“只影响文本显示、不影响 DOM 结构”的扩展命名实体集合。

## 代码改造点（最小侵入）
1. **把 `decode_html_entities` 改成单次扫描实现**
   - 位置：[string_utils.hpp](file:///e:/Hunlongyu/html_parser/include/hps/utils/string_utils.hpp#L246-L269)
   - 当前实现是 6 个实体做多轮 `find/replace`（O(n*m)）。改成：从左到右扫描，遇到 `&` 尝试解析：
     - 先匹配最小命名实体集合（固定几种，常量时间比较）。
     - 再尝试数字实体（`&#` / `&#x`），按严格规则解析并写入 UTF-8。
     - 解析失败则原样输出 `&`，继续扫描。
2. **默认行为调整：结构优先、解码可选**
   - 位置：[options.hpp](file:///e:/Hunlongyu/html_parser/include/hps/parsing/options.hpp#L224-L230)
   - 将 `text_processing_mode` 默认从 `Decode` 改为 `Raw`，并修正该行注释与默认值的不一致。
   - TreeBuilder 逻辑保持不变：[tree_builder.cpp](file:///e:/Hunlongyu/html_parser/src/parsing/tree_builder.cpp#L106-L134)
     - 用户显式开启 `Decode` 时才调用解码函数。
3. **不改变 Tokenizer/属性值解析**
   - 因为你的目标是“结构解析优先”，且属性值的字符引用解码不会影响结构；保持当前“属性值原样”策略，避免扩大行为变更面。

## 测试补齐（验证不影响结构 + 解码最小集合生效）
1. 新增一个解析级测试可执行文件（例如 `parsing_entity_tests`）：
   - 使用 [HTMLParser](file:///e:/Hunlongyu/html_parser/include/hps/parsing/html_parser.hpp#L14-L68) 解析 HTML 字符串。
2. 覆盖用例：
   - TextNode：`"Tom &amp; Jerry"` → `Tom & Jerry`（在 `Decode` 模式下）。
   - 数字实体：`"&#65;&#x41;"` → `AA`（在 `Decode` 模式下）。
   - 严格失败原样保留：`"&#xZZ;"`、`"&#65"`（缺分号）不解码。
   - 结构不受影响：`"&lt;div id=\"x\"&gt;"` 仍为 TextNode，不产生 Element。

## 验证方式
- 运行全部 gtest 测试用例（现有 core_* + 新增 parsing_entity_tests）。
- 用一组手工片段回归，确认默认 `Raw` 不做任何文本替换。