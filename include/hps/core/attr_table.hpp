#pragma once
// 内部头（不随安装导出）：属性名整数化（lexbor 风格 attr-id）。
//
// 与 tag_table 同构但更简单（属性名无分类标志）：已知属性名构成排序主表，id = 下标 + 1
// （0 = Attr::Unknown）。未知/自定义属性（data-* / aria-* / 自定义）解析为 Unknown，
// 由调用方回退按名比较。编译期 static_assert 保证主表有序、命名常量均可解析。
#include "hps/hps_fwd.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace hps::attr {

// 已知属性名（小写，ASCII 升序）。覆盖 HTML 常见全局/元素属性。
inline constexpr auto kNames = std::to_array<std::string_view>({
    "abbr", "accept", "accept-charset", "accesskey", "action", "align", "allow", "alt", "async",
    "autocapitalize", "autocomplete", "autofocus", "autoplay",
    "background", "bgcolor", "border",
    "capture", "cellpadding", "cellspacing", "charset", "checked", "cite", "class", "color",
    "cols", "colspan", "content", "contenteditable", "controls", "coords", "crossorigin",
    "data", "datetime", "decoding", "default", "defer", "dir", "dirname", "disabled", "download",
    "draggable",
    "enctype", "enterkeyhint",
    "face", "for", "form", "formaction", "formenctype", "formmethod", "formnovalidate",
    "formtarget", "frameborder",
    "headers", "height", "hidden", "high", "href", "hreflang", "http-equiv",
    "id", "inputmode", "integrity", "is", "ismap",
    "kind",
    "label", "lang", "list", "loading", "longdesc", "loop", "low",
    "max", "maxlength", "media", "method", "min", "minlength", "multiple", "muted",
    "name", "nomodule", "nonce", "novalidate",
    "open", "optimum",
    "pattern", "ping", "placeholder", "playsinline", "poster", "preload", "property",
    "readonly", "referrerpolicy", "rel", "required", "reversed", "role", "rows", "rowspan",
    "sandbox", "scope", "scrolling", "selected", "shape", "size", "sizes", "slot", "span",
    "spellcheck", "src", "srcdoc", "srclang", "srcset", "start", "step", "style",
    "tabindex", "target", "title", "translate", "type",
    "usemap",
    "valign", "value",
    "width", "wrap",
});

static_assert(std::ranges::is_sorted(kNames), "attr::kNames 必须按 ASCII 升序排列");

inline constexpr std::size_t kKnownCount = kNames.size();

// 名字 → Attr（传小写）。
[[nodiscard]] constexpr Attr from_lower(std::string_view name) noexcept {
    const auto it = std::lower_bound(kNames.begin(), kNames.end(), name);
    if (it != kNames.end() && *it == name) {
        return static_cast<Attr>((it - kNames.begin()) + 1);
    }
    return Attr::Unknown;
}

// 名字 → Attr，大小写不敏感（HTML 属性名大小写不敏感）。
[[nodiscard]] inline Attr from_name_ci(std::string_view name) noexcept {
    char buf[64];
    if (name.size() > sizeof(buf)) {
        return Attr::Unknown;
    }
    for (std::size_t i = 0; i < name.size(); ++i) {
        const char c = name[i];
        buf[i]       = (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    }
    return from_lower(std::string_view{buf, name.size()});
}

[[nodiscard]] constexpr bool is_known(Attr a) noexcept {
    const auto id = static_cast<std::uint16_t>(a);
    return id != 0 && id <= kKnownCount;
}

[[nodiscard]] constexpr std::string_view static_name(Attr a) noexcept {
    return is_known(a) ? kNames[static_cast<std::size_t>(static_cast<std::uint16_t>(a)) - 1]
                       : std::string_view{};
}

// 命名常量（供按名比较的热点：id / class / charset 等）。
inline constexpr Attr kId        = from_lower("id");
inline constexpr Attr kClass     = from_lower("class");
inline constexpr Attr kHref      = from_lower("href");
inline constexpr Attr kSrc       = from_lower("src");
inline constexpr Attr kName      = from_lower("name");
inline constexpr Attr kCharset   = from_lower("charset");
inline constexpr Attr kHttpEquiv = from_lower("http-equiv");
inline constexpr Attr kContent   = from_lower("content");
inline constexpr Attr kProperty  = from_lower("property");
inline constexpr Attr kType      = from_lower("type");
inline constexpr Attr kStyle     = from_lower("style");

static_assert(kId != Attr::Unknown && kClass != Attr::Unknown && kHref != Attr::Unknown &&
                  kSrc != Attr::Unknown && kName != Attr::Unknown && kCharset != Attr::Unknown &&
                  kHttpEquiv != Attr::Unknown && kContent != Attr::Unknown &&
                  kProperty != Attr::Unknown && kType != Attr::Unknown && kStyle != Attr::Unknown,
              "命名属性常量未能在主表解析——检查 attr::kNames");

}  // namespace hps::attr
