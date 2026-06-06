#pragma once
// 内部头（不随安装导出）：标签名整数化（lexbor 风格 tag-id）。
//
// 设计：已知标签名构成一张排序主表 kNames，id = 下标 + 1（0 = Tag::Unknown）；
// 分类标志表 kFlags 由“真值数组”（与 tree_builder 历史口径一一对应）在编译期派生，
// 因此整数分发与原字符串判定**行为等价**。编译期 static_assert 保证主表有序、且每个
// 真值数组条目都能在主表解析到——漏写标签会直接编译失败，杜绝悄无声息的一致性回退。
#include "hps/hps_fwd.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace hps::tag {

// ───────────────────────── 主表：已知标签名（小写，ASCII 升序）─────────────────────────
inline constexpr auto kNames = std::to_array<std::string_view>({
    "a", "abbr", "address", "annotation-xml", "applet", "area", "article", "aside", "audio",
    "b", "base", "basefont", "bdi", "bdo", "bgsound", "big", "blockquote", "body", "br", "button",
    "canvas", "caption", "center", "cite", "code", "col", "colgroup",
    "data", "datalist", "dd", "del", "desc", "details", "dfn", "dialog", "dir", "div", "dl", "dt",
    "em", "embed",
    "fieldset", "figcaption", "figure", "font", "footer", "foreignobject", "form", "frame", "frameset",
    "h1", "h2", "h3", "h4", "h5", "h6", "head", "header", "hgroup", "hr", "html",
    "i", "iframe", "img", "input", "ins",
    "kbd", "keygen",
    "label", "legend", "li", "link", "listing",
    "main", "map", "mark", "marquee", "math", "menu", "meta", "meter", "mi", "mn", "mo", "ms", "mtext",
    "nav", "nobr", "noembed", "noframes", "noscript",
    "object", "ol", "optgroup", "option", "output",
    "p", "param", "picture", "plaintext", "pre", "progress",
    "q", "rb", "rp", "rt", "rtc", "ruby",
    "s", "samp", "script", "section", "select", "slot", "small", "source", "span", "strike", "strong",
    "style", "sub", "summary", "sup", "svg",
    "table", "tbody", "td", "template", "textarea", "tfoot", "th", "thead", "time", "title", "tr", "track", "tt",
    "u", "ul", "var", "video", "wbr", "xmp",
});

static_assert(std::ranges::is_sorted(kNames), "kNames 必须按 ASCII 升序排列（用于二分与去重）");

inline constexpr std::size_t kKnownCount = kNames.size();

// 名字 → Tag（调用方需传小写；HTML 词法器已把标签名小写化）。
[[nodiscard]] constexpr Tag from_lower(std::string_view name) noexcept {
    const auto it = std::lower_bound(kNames.begin(), kNames.end(), name);
    if (it != kNames.end() && *it == name) {
        return static_cast<Tag>((it - kNames.begin()) + 1);
    }
    return Tag::Unknown;
}

// 名字 → Tag，大小写不敏感（用于 preserve_case 名字与公共 API）。标签名很短，
// 在栈缓冲里做 ASCII 小写化后查表；超长名必非已知标签。
[[nodiscard]] inline Tag from_name_ci(std::string_view name) noexcept {
    char buf[64];
    if (name.size() > sizeof(buf)) {
        return Tag::Unknown;
    }
    for (std::size_t i = 0; i < name.size(); ++i) {
        const char c = name[i];
        buf[i]       = (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    }
    return from_lower(std::string_view{buf, name.size()});
}

[[nodiscard]] constexpr bool is_known(Tag t) noexcept {
    const auto id = static_cast<std::uint16_t>(t);
    return id != 0 && id <= kKnownCount;
}

[[nodiscard]] constexpr std::size_t index_of(Tag t) noexcept {
    return static_cast<std::size_t>(static_cast<std::uint16_t>(t)) - 1;  // 仅对已知 tag 有效
}

// 已知 tag 的静态名字视图（静态存储，永久有效）；未知/自定义返回空。
[[nodiscard]] constexpr std::string_view static_name(Tag t) noexcept {
    return is_known(t) ? kNames[index_of(t)] : std::string_view{};
}

// ───────────────────────── 分类标志 ─────────────────────────
enum Cat : std::uint32_t {
    CatSpecial          = 1u << 0,
    CatFormatting       = 1u << 1,
    CatHeadContent      = 1u << 2,
    CatTableSection     = 1u << 3,  // tbody/tfoot/thead
    CatTableCell        = 1u << 4,  // td/th
    CatHeading          = 1u << 5,  // h1..h6
    CatTableStructure   = 1u << 6,  // caption/col/colgroup/table/tbody/td/tfoot/th/thead/tr
    CatTableContainer   = 1u << 7,  // caption/colgroup
    CatOptionalEnd      = 1u << 8,  // EOF 可省略闭合
    CatMarkerScope      = 1u << 9,  // 压 active-formatting marker
    CatForeignBreakout  = 1u << 10, // 外来内容 breakout 起始标签
    CatPCloser          = 1u << 11, // 触发隐式关闭 <p>
};

namespace detail {
// 真值数组（口径与 tree_builder 历史一致）。构建标志表只在编译期跑一次，用线性查找
// 即可（无需这些数组有序）。
inline constexpr auto kSpecial = std::to_array<std::string_view>({
    "address", "applet", "area", "article", "aside", "base", "basefont", "bgsound",
    "blockquote", "body", "br", "button", "caption", "center", "col", "colgroup", "dd",
    "details", "dir", "div", "dl", "dt", "embed", "fieldset", "figcaption", "figure",
    "footer", "form", "frame", "frameset", "h1", "h2", "h3", "h4", "h5", "h6", "head",
    "header", "hgroup", "hr", "html", "iframe", "img", "input", "keygen", "li", "link",
    "listing", "main", "marquee", "menu", "meta", "nav", "noembed", "noframes", "noscript",
    "object", "ol", "p", "param", "plaintext", "pre", "script", "section", "select",
    "source", "style", "summary", "table", "tbody", "td", "template", "textarea", "tfoot",
    "th", "thead", "title", "tr", "track", "ul", "wbr", "xmp"});
inline constexpr auto kFormatting = std::to_array<std::string_view>({
    "a", "b", "big", "code", "em", "font", "i", "nobr", "s", "small", "strike", "strong", "tt", "u"});
inline constexpr auto kHeadContent = std::to_array<std::string_view>({
    "base", "basefont", "bgsound", "link", "meta", "noframes", "noscript", "script", "style", "template", "title"});
inline constexpr auto kTableSection = std::to_array<std::string_view>({"tbody", "tfoot", "thead"});
inline constexpr auto kTableCell    = std::to_array<std::string_view>({"td", "th"});
inline constexpr auto kHeading      = std::to_array<std::string_view>({"h1", "h2", "h3", "h4", "h5", "h6"});
inline constexpr auto kTableStruct  = std::to_array<std::string_view>({
    "caption", "col", "colgroup", "table", "tbody", "td", "tfoot", "th", "thead", "tr"});
inline constexpr auto kTableCont    = std::to_array<std::string_view>({"caption", "colgroup"});
inline constexpr auto kOptionalEnd  = std::to_array<std::string_view>({
    "body", "caption", "colgroup", "dd", "dt", "head", "html", "li", "optgroup", "option",
    "p", "rb", "rp", "rt", "rtc", "tbody", "td", "tfoot", "th", "thead", "tr"});
inline constexpr auto kMarkerScope  = std::to_array<std::string_view>({
    "td", "th", "caption", "applet", "marquee", "object", "template"});
inline constexpr auto kBreakout     = std::to_array<std::string_view>({
    "b", "big", "blockquote", "body", "br", "center", "code", "dd", "div", "dl", "dt",
    "em", "embed", "h1", "h2", "h3", "h4", "h5", "h6", "head", "hr", "i", "img", "li",
    "listing", "menu", "meta", "nobr", "ol", "p", "pre", "ruby", "s", "small", "span",
    "strike", "strong", "sub", "sup", "table", "tt", "u", "ul", "var"});
inline constexpr auto kPCloser      = std::to_array<std::string_view>({
    "address", "article", "aside", "blockquote", "center", "dd", "details", "dialog",
    "dir", "div", "dl", "dt", "fieldset", "figcaption", "figure", "footer", "form",
    "h1", "h2", "h3", "h4", "h5", "h6", "header", "hgroup", "hr", "listing", "main",
    "menu", "nav", "ol", "p", "plaintext", "pre", "section", "summary", "table", "ul", "xmp"});

constexpr bool contains(std::span<const std::string_view> arr, std::string_view n) {
    return std::ranges::find(arr, n) != arr.end();
}

constexpr std::array<std::uint32_t, kKnownCount> build_flags() {
    std::array<std::uint32_t, kKnownCount> f{};
    for (std::size_t i = 0; i < kKnownCount; ++i) {
        const std::string_view n = kNames[i];
        std::uint32_t          v = 0;
        if (contains(kSpecial, n)) v |= CatSpecial;
        if (contains(kFormatting, n)) v |= CatFormatting;
        if (contains(kHeadContent, n)) v |= CatHeadContent;
        if (contains(kTableSection, n)) v |= CatTableSection;
        if (contains(kTableCell, n)) v |= CatTableCell;
        if (contains(kHeading, n)) v |= CatHeading;
        if (contains(kTableStruct, n)) v |= CatTableStructure;
        if (contains(kTableCont, n)) v |= CatTableContainer;
        if (contains(kOptionalEnd, n)) v |= CatOptionalEnd;
        if (contains(kMarkerScope, n)) v |= CatMarkerScope;
        if (contains(kBreakout, n)) v |= CatForeignBreakout;
        if (contains(kPCloser, n)) v |= CatPCloser;
        f[i] = v;
    }
    return f;
}

// 完整性：每个真值数组的条目都必须在主表里（否则其 tag 会被当未知，分类丢失 → 一致性回退）。
constexpr bool all_resolve(std::span<const std::string_view> arr) {
    for (auto n : arr) {
        if (from_lower(n) == Tag::Unknown) return false;
    }
    return true;
}
}  // namespace detail

inline constexpr std::array<std::uint32_t, kKnownCount> kFlags = detail::build_flags();

static_assert(detail::all_resolve(detail::kSpecial), "kSpecial 有标签不在主表 kNames 中");
static_assert(detail::all_resolve(detail::kFormatting), "kFormatting 有标签不在主表中");
static_assert(detail::all_resolve(detail::kHeadContent), "kHeadContent 有标签不在主表中");
static_assert(detail::all_resolve(detail::kTableStruct), "kTableStruct 有标签不在主表中");
static_assert(detail::all_resolve(detail::kOptionalEnd), "kOptionalEnd 有标签不在主表中");
static_assert(detail::all_resolve(detail::kMarkerScope), "kMarkerScope 有标签不在主表中");
static_assert(detail::all_resolve(detail::kBreakout), "kBreakout 有标签不在主表中");
static_assert(detail::all_resolve(detail::kPCloser), "kPCloser 有标签不在主表中");

[[nodiscard]] constexpr std::uint32_t flags_of(Tag t) noexcept {
    return is_known(t) ? kFlags[index_of(t)] : 0u;
}

// 分类判定（整数位测试取代字符串比较）。
[[nodiscard]] constexpr bool is_special(Tag t) noexcept { return (flags_of(t) & CatSpecial) != 0; }
[[nodiscard]] constexpr bool is_formatting(Tag t) noexcept { return (flags_of(t) & CatFormatting) != 0; }
[[nodiscard]] constexpr bool is_head_content(Tag t) noexcept { return (flags_of(t) & CatHeadContent) != 0; }
[[nodiscard]] constexpr bool is_table_section(Tag t) noexcept { return (flags_of(t) & CatTableSection) != 0; }
[[nodiscard]] constexpr bool is_table_cell(Tag t) noexcept { return (flags_of(t) & CatTableCell) != 0; }
[[nodiscard]] constexpr bool is_heading(Tag t) noexcept { return (flags_of(t) & CatHeading) != 0; }
[[nodiscard]] constexpr bool is_table_structure(Tag t) noexcept { return (flags_of(t) & CatTableStructure) != 0; }
[[nodiscard]] constexpr bool is_table_container(Tag t) noexcept { return (flags_of(t) & CatTableContainer) != 0; }
[[nodiscard]] constexpr bool is_optional_end(Tag t) noexcept { return (flags_of(t) & CatOptionalEnd) != 0; }
[[nodiscard]] constexpr bool is_marker_scope(Tag t) noexcept { return (flags_of(t) & CatMarkerScope) != 0; }
[[nodiscard]] constexpr bool is_foreign_breakout(Tag t) noexcept { return (flags_of(t) & CatForeignBreakout) != 0; }
[[nodiscard]] constexpr bool is_p_closer(Tag t) noexcept { return (flags_of(t) & CatPCloser) != 0; }

// ───────────────────────── 命名常量（供按名比较的热点使用）─────────────────────────
inline constexpr Tag kHtml          = from_lower("html");
inline constexpr Tag kHead          = from_lower("head");
inline constexpr Tag kBody          = from_lower("body");
inline constexpr Tag kFrameset      = from_lower("frameset");
inline constexpr Tag kFrame         = from_lower("frame");
inline constexpr Tag kNoframes      = from_lower("noframes");
inline constexpr Tag kTable         = from_lower("table");
inline constexpr Tag kTr            = from_lower("tr");
inline constexpr Tag kCaption       = from_lower("caption");
inline constexpr Tag kColgroup      = from_lower("colgroup");
inline constexpr Tag kCol           = from_lower("col");
inline constexpr Tag kTemplate      = from_lower("template");
inline constexpr Tag kSelect        = from_lower("select");
inline constexpr Tag kOption        = from_lower("option");
inline constexpr Tag kOptgroup      = from_lower("optgroup");
inline constexpr Tag kForm          = from_lower("form");
inline constexpr Tag kA             = from_lower("a");
inline constexpr Tag kNobr          = from_lower("nobr");
inline constexpr Tag kP             = from_lower("p");
inline constexpr Tag kLi            = from_lower("li");
inline constexpr Tag kDd            = from_lower("dd");
inline constexpr Tag kDt            = from_lower("dt");
inline constexpr Tag kButton        = from_lower("button");
inline constexpr Tag kBr            = from_lower("br");
inline constexpr Tag kTbody         = from_lower("tbody");
inline constexpr Tag kApplet        = from_lower("applet");
inline constexpr Tag kMarquee       = from_lower("marquee");
inline constexpr Tag kObject        = from_lower("object");
inline constexpr Tag kSvg           = from_lower("svg");
inline constexpr Tag kMath          = from_lower("math");
inline constexpr Tag kForeignObject = from_lower("foreignobject");
inline constexpr Tag kDesc          = from_lower("desc");
inline constexpr Tag kTitle         = from_lower("title");
inline constexpr Tag kMi            = from_lower("mi");
inline constexpr Tag kMo            = from_lower("mo");
inline constexpr Tag kMn            = from_lower("mn");
inline constexpr Tag kMs            = from_lower("ms");
inline constexpr Tag kMtext         = from_lower("mtext");
inline constexpr Tag kAnnotationXml = from_lower("annotation-xml");

}  // namespace hps::tag
