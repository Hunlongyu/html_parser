# HPS 0.3.0 设计文档：lexbor 风格的名字驻留与字符串 arena

状态：**设计草案（未实现）** · 目标版本：0.3.0（破坏性）· 日期：2026-06-06

---

## 0. 背景与目标

### 0.1 来自 profile 的事实（0.2.0，arena 之后）

`perf_probe`（覆写全局 `new/delete` 计数 + tokenize/full 分相计时，best-of-N `min_ms`）：

| 文件 | 节点 | full_ms | tok_ms | build_ms | full 分配 | tok 分配 | build 分配 | a/node |
|---|---|---|---|---|---|---|---|---|
| fofa.html | 4740 | 13.15 | 5.09 | **8.06** | 19807 | 7995 | 11812 | 4.18 |
| shodan.html | 697 | 1.19 | 0.52 | **0.67** | 4508 | 1904 | 2604 | 6.47 |
| panlong.html | 539 | 0.52 | 0.19 | **0.33** | 1521 | 656 | 865 | 2.82 |

结论：
1. **tree-builder 主导**：占解析时间 56–73%、分配 57–60%。
2. tree-builder 的时间里，**分配只占约 1/4–1/3**，其余是 CPU 逻辑——而那些逻辑几乎全是对**标签名做 `equals_ignore_case` 字符串比较**（scope 检查、隐式闭合、AAA、`is_special_element`/`is_formatting_element` 等大数组扫描）。
3. arena 已生效：a/node 从 ~6 降到 2.8–6.5。剩余分配集中在 32–256 字节桶 = 字符串堆缓冲 + 属性向量。

### 0.2 目标

用**现代 C++** 复刻 lexbor 的高性能 DOM 架构。lexbor 之所以快 3–10×，核心不是「C 比 C++ 快」，而是 **「名字 = 整数」**：标签/属性名驻留成整数 id，整个建树状态机用整数分发，名字字符串只在需要时查表。

三件套：

| # | 支柱 | 现状 |
|---|---|---|
| 1 | 节点 arena / 对象池 | ✅ 0.2.0 已完成（`NodeArena`） |
| 2 | **名字驻留成整数**（tag-id + 属性名 id） | ❌ 本设计 Phase 1 / 2 |
| 3 | **字符串 arena**（文本/属性值 = 指向池的 `string_view`） | ❌ 本设计 Phase 3 |

### 0.3 约束 / 非目标

- 一致性必须保持 **1227/1764**（每阶段跑完整 html5lib 套件核对）。
- 公共 API 一律 snake_case；公共头不含内部头/平台头。
- 自给自足：**不引第三方库**（不引 `frozen`/`gperf`）——用手写 `constexpr` 排序数组，沿用项目现有「排序数组 + `binary_search`」惯用法。
- 不做 SIMD/汇编；`parse()` 外形不变（仍返回 `shared_ptr<Document>`）。
- 不引入 GC；所有权仍是「Document 单一所有者」。

---

## 1. 现有积木 → lexbor 对照

| lexbor (C) | 作用 | HPS (现代 C++) | 现状 |
|---|---|---|---|
| `lexbor_dobject`（定长对象池） | 节点分配 | `NodeArena`（单调 bump + RAII 虚析构） | ✅ 已有 |
| `lexbor_mraw`（变长内存池） | 文本/属性值 | `StringPool`（已存在，CSS 子系统在用） | ✅ 已有，待接入 DOM |
| `lxb_tag_id_t` enum + 生成表 | 标签整数化 | `enum class Tag : uint16_t` + `constexpr` 表 | ❌ Phase 1 |
| `lexbor_hash` 自定义 tag 堆 | 未知标签驻留 | per-Document 驻留表（键存 StringPool） | ❌ Phase 1 |
| 生成的 C 位标志表 | tag 分类 | `constexpr std::array<TagCat, N>` | ❌ Phase 1 |
| `(char*, size_t)` | 字符串引用 | `std::string_view` | 部分（token 已是） |

---

## 2. 总体架构

### 2.1 所有权（Document 持有这些，节点/视图皆随其生命周期）

```
Document
 ├─ NodeArena      m_arena;        // 全部节点（已有）
 ├─ StringPool     m_strings;      // 文本 / 属性值 / 自定义名字（Phase 3 接入）
 ├─ std::string    m_html_source;  // 已 decode 的源（已有）—— 逐字子串可零拷贝
 ├─ TagInterner    m_tags;         // 自定义标签名 → Tag（Phase 1）
 └─ AttrInterner   m_attrs;        // 自定义属性名 → Attr（Phase 2）
```

### 2.2 数据流

```
源字节 → decode(UTF-8) ─┐
                         ├─► Tokenizer ──► Token{ Tag tag_id; string_view name/value; ... }
已知标签静态表 ──────────┘                    │  (已知标签直接解析成 id；未知留 Unknown)
                                              ▼
                                         TreeBuilder（整数分发：switch(tag) / flags[tag] & Cat）
                                              │  未知标签在插入时经 Document 驻留拿到自定义 id
                                              ▼
                                         DOM 节点：{ Tag m_tag; string_view m_name; ... }
```

### 2.3 关键设计：节点同时存 id 和 name 视图

仿 lexbor「tag_id 管分发、name 指针管取回」。Element 同时持：

```cpp
Tag              m_tag;   //  快速整数分发（建树/CSS 匹配热路径）
std::string_view m_name;  //  取回名字：已知→静态表；自定义→StringPool（两者都恒有效）
NamespaceKind    m_ns;
```

于是 `tag_name()` = `return m_name;`（**无分支、无 owner_document 走链**），`tag()` = `return m_tag;`。
比 0.2.0 的 `std::string m_name`（32B）更小：`Tag(2)+ns(1)+string_view(16)`，且名字字符串零拷贝。

---

## 3. Phase 1 — Tag 整数化（收益最大，先做）

### 3.1 `Tag` 枚举

```cpp
enum class Tag : uint16_t {
    Unknown = 0,
    // 1..K：HTML 标准已知元素（来自 HTML 标准元素清单，~140 个）
    A, Abbr, /* … */ Div, /* … */ Table, Tbody, Td, /* … */
    KnownCount,            // = K+1
    CustomBase = KnownCount  // 运行期自定义标签从此递增
};
```

- 取值布局：`0` = Unknown；`[1, KnownCount)` = 已知；`[CustomBase, …)` = 自定义（每个 Document 内递增）。
- 命名空间正交：SVG/MathML 元素 + 未知标签走「自定义驻留」（不进 HTML 标志表）；Element 同时存 `Tag` + `NamespaceKind`。Phase 1 只整数化 **HTML 命名空间已知标签**，外来元素行为不变。

### 3.2 名字 ↔ id 映射

```cpp
// 已知：编译期排序数组 + lower_bound（tokenizer 已把 HTML 标签小写化）
constexpr std::array<std::pair<std::string_view, Tag>, K> kKnownByName = /* 按名排序 */;
constexpr Tag         tag_from_known(std::string_view lower) noexcept; // 命中→id，否则 Unknown
constexpr std::string_view name_of_known(Tag) noexcept;               // id→静态 string_view
```

- **自定义驻留**（per-Document，单线程无锁）：
  ```cpp
  class TagInterner {
      std::unordered_map<std::string_view, Tag> m_by_name; // 键指向 StringPool
      std::vector<std::string_view>             m_by_id;   // id - CustomBase → name
  public:
      Tag intern(std::string_view name, StringPool&); // 已在表→复用；否则存池 + 分配新 id
      std::string_view name_of(Tag) const;
  };
  ```

### 3.3 标志表（取代散落的成员判定）—— **CPU 主战场**

```cpp
enum class TagCat : uint32_t {
    None         = 0,
    Special      = 1u << 0,
    Formatting   = 1u << 1,
    HeadContent  = 1u << 2,
    TableSection = 1u << 3,  // tbody/tfoot/thead
    TableCell    = 1u << 4,  // td/th
    Heading      = 1u << 5,  // h1..h6
    Void         = 1u << 6,  // br/img/...
    ClosesP      = 1u << 7,  // 触发隐式关闭 <p>
    OptionalEnd  = 1u << 8,  // EOF 可省略闭合
    MarkerScope  = 1u << 9,  // 压 marker（td/th/caption/applet/...）
    Breakout     = 1u << 10, // 外来内容 breakout
    // …按需补充
};
constexpr std::array<std::uint32_t, K> kTagCat = /* constexpr 构建 */;

inline bool is_special(Tag t)    noexcept { return t < Tag::KnownCount && (kTagCat[idx(t)] & u(TagCat::Special)); }
inline bool is_formatting(Tag t) noexcept { return t < Tag::KnownCount && (kTagCat[idx(t)] & u(TagCat::Formatting)); }
// …
```

**待迁移的判定函数**（现为字符串比较 / 排序数组扫描 → 改为 `kTagCat[id] & Cat`）：
`is_special_element`、`is_formatting_element`、`is_head_content_tag`、`is_table_section_tag`、
`is_heading_tag`、`is_table_cell_tag`、`is_table_structure_tag`、`is_table_container_tag`、
`is_void_element`、`can_omit_end_tag_at_eof`、`is_marker_scope_tag`、`is_foreign_breakout_tag`、
`check_implicit_close` 的 `p_closers` 表、`is_formatting_element` 的 formatting 列表。

未知/自定义 tag（`id >= CustomBase`）默认 `None`（非 special/非 formatting）——符合 HTML「未知元素按普通元素处理」。

### 3.4 Element 改造

- 成员：`std::string m_name` → `Tag m_tag` + `std::string_view m_name`（见 §2.3）。
- `tag_name()`：`const std::string&` → **`std::string_view`**（破坏）；新增 `tag() -> Tag`。
- 建树中所有 `equals_ignore_case(el->tag_name(), "table")` → `el->tag() == Tag::Table`。

### 3.5 Tokenizer 改造

- Token 增 `Tag m_tag_id`；产出开始/结束标签时，把小写化名字经 `tag_from_known` 解析；未知 → `Tag::Unknown`（tokenizer **不依赖 Document**）。
- TreeBuilder 在 `create_element` 时：若 `Tag::Unknown` 或外来命名空间 → 调 `m_document` 的 `TagInterner` 驻留拿自定义 id 与稳定 name 视图；否则用已知 id + 静态 name。

### 3.6 CSS 匹配器改造

- 类型选择器 `div`：在**解析选择器时**把类型名解析为 `Tag`（已知）或标记为「自定义名」（保留字符串）。
- 匹配：已知 → `el.tag() == sel.tag`（整数）；自定义 → 回退按名比较（或对目标 Document 驻留表查 id）。`*` 通配不变。
- 大小写：HTML 选择器大小写不敏感 → id 归一化天然解决。

### 3.7 验证

完整 ctest + 1764 一致性必须 **1227**；`perf_probe` 量 before/after（预期 `build_ms` 显著下降）。

---

## 4. Phase 2 — 属性名驻留

- `enum class Attr : uint16_t { Unknown, Id, Class, Href, Src, /*…常见~80*/ KnownCount, CustomBase }` + `AttrInterner`（同 §3.2）。
- `Attribute` 存 `Attr m_id` + `string_view m_name`（同 §2.3）+ value。
- `get_attribute`/`has_attribute`：线性 `equals_ignore_case` 扫描 → `attr.id() == Attr::Href` 整数比较。
- CSS：`[href]` 属性选择器、`#id`、`.class` —— 属性**名**比较整数化；`id`/`class` 已被查询索引缓存覆盖。
- API：`Attribute::name()` `const std::string&` → `std::string_view`（破坏）。
- 注意：`class`/`id` 的**值**仍是字符串，Phase 3 处理。

---

## 5. Phase 3 — 字符串 arena

- `TextNode::value()`、`Attribute::value()`、`CommentNode::value()` → `std::string_view`（指向源或 StringPool）。
- TreeBuilder 插入文本/属性值时：
  - **逐字子串**（无实体解码、无空白规范化、无大小写调整）→ `string_view` 直接指向 Document 的 `m_html_source`（**零拷贝**）。
  - 否则（实体解码 `&amp;`→`&`、规范化、SVG 属性 camelCase）→ `m_strings.add()` 取稳定视图。
- `Document` 须在其拥有的 view 使用者之前不被销毁——已由所有权保证（节点、池、源都在 Document 内）。
- 修改语义：`add_attribute` 写新值 → 入池；覆盖旧值时旧串留池中（轻微浪费，可接受，符合 monotonic arena 取舍）。
- API：`value()`/`CommentNode::value()` `const std::string&` → `std::string_view`（破坏）。`text_content()`/`own_text()` 仍按值返回 `std::string`（本就拼接）。

> ⚠️ 这正是 0.2.0 否决的「非拥有 view」——但区别在于：**view 指向 Document 自己拥有的池/源（单一所有者，与节点同生命周期），不是指向调用方的临时缓冲**。安全前提是「解码后的内容存在 Document 内」，对实体/转码/大小写修正一律走池拷贝，不强行指向不存在的源子串。

---

## 6. API 破坏清单（0.3.0）

| 接口 | 0.2.0 | 0.3.0 | 阶段 |
|---|---|---|---|
| `Element::tag_name()` | `const std::string&` | `std::string_view` | 1 |
| `Element::tag()` | — | `Tag`（新增） | 1 |
| `Element::id()` / `class_name()` | `const std::string&` | `std::string_view` | 2/3 |
| `Element::get_attribute()` | `const std::string&` | `std::string_view` | 3 |
| `Attribute::name()` / `value()` | `const std::string&` | `std::string_view` | 2 / 3 |
| `TextNode::value()` / `CommentNode::value()` | `const std::string&` | `std::string_view` | 3 |
| `text_content()` / `own_text()` | `std::string`（值） | 不变 | — |

`string_view` 多数用法（打印、比较、传 `string_view` 形参）源码兼容；**持久保存引用/拼接**的调用方需调整。须更新 `docs/API.md`、`CHANGELOG.md`、迁移指南，并 bump 0.3.0。

---

## 7. 风险与缓解

| 风险 | 缓解 |
|---|---|
| 一致性回退（tag 表不全 / 大小写 / 外来命名空间） | 每阶段跑 1764 套件守住 1227；tag 表以 HTML 标准元素清单为准逐项核对 |
| SVG/MathML camelCase（`viewBox` 等） | 外来元素走自定义驻留，**不进** HTML 标志表 → 行为不变 |
| 自定义元素（`<my-widget>`） | 走自定义驻留，按普通元素处理（id ≥ CustomBase，标志 None） |
| `string_view` 悬空 | 单一所有者：池/源/节点同属 Document；销毁顺序由成员声明保证（arena/池最后析构） |
| 线程安全 | Document 单线程解析，驻留表无锁；文档化「Document 非线程安全」 |
| CSS 自定义类型选择器 | 解析期解析 id，未知名保留字符串回退路径 |

---

## 8. 实施顺序与预期

1. **Phase 1（tag 整数化 + 标志表）** — 直击 build_ms 的 CPU 主成本，单项收益最大，**先做**。
2. **Phase 2（属性名驻留）** — 削属性查找/CSS 属性匹配的字符串比较。
3. **Phase 3（字符串 arena）** — 削 32–256B 字符串分配，逐字处零拷贝。

每阶段：独立 commit → 完整 ctest → 1764 一致性 1227 → `perf_probe` before/after。

诚实预期：Phase 1 是「拉向 lexbor 量级」的关键一步（整数分发取代字符串比较）；Phase 2/3 主要降分配与剩余比较。组合后有望把 `build_ms` 明显压下来，但具体倍率需 `perf_probe` 实测，不在此预设数字。

---

## 9. 明确不做

- 不引第三方（`frozen`/`gperf`）：手写 `constexpr` 排序数组。
- 不做 SIMD/汇编、不改 `parse()` 外形、不引 GC。
- 外来元素（SVG/MathML）Phase 1 不整数化，保持现有较轻量处理。
