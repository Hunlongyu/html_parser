#include "serializer.hpp"

#include "hps/core/attribute.hpp"
#include "hps/core/comment_node.hpp"
#include "hps/core/element.hpp"
#include "hps/core/node.hpp"
#include "hps/core/text_node.hpp"
#include "hps/utils/string_utils.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace hps::detail {
namespace {

// HTML5 void 元素：仅输出起始标签，无内容、无闭合标签。
constexpr std::array<std::string_view, 16> void_tags = {
    "area", "base", "br", "col", "embed", "hr", "img", "input",
    "link", "meta", "param", "source", "track", "wbr", "basefont", "frame"};

// raw text 元素：子文本原样输出（不转义），常见于 <script>/<style>。
constexpr std::array<std::string_view, 8> raw_text_tags = {
    "script", "style", "xmp", "iframe", "noembed", "noframes", "noscript", "plaintext"};

[[nodiscard]] bool tag_in(const std::array<std::string_view, 16>& set, std::string_view tag) {
    return std::ranges::any_of(set, [tag](const std::string_view name) {
        return equals_ignore_case(tag, name);
    });
}

[[nodiscard]] bool tag_in(const std::array<std::string_view, 8>& set, std::string_view tag) {
    return std::ranges::any_of(set, [tag](const std::string_view name) {
        return equals_ignore_case(tag, name);
    });
}

[[nodiscard]] bool is_void_tag(const std::string_view tag) {
    return tag_in(void_tags, tag);
}

[[nodiscard]] bool is_raw_text_tag(const std::string_view tag) {
    return tag_in(raw_text_tags, tag);
}

// 判断位置 i 处的 '&' 是否已是一个字符引用（&name;、&#123;、&#xAF;）。
// 用于实体感知转义：源文本未解码实体时（默认 Zero-Copy 模式）避免二次转义。
[[nodiscard]] bool looks_like_char_reference(const std::string_view text, const size_t amp_pos) {
    size_t       i        = amp_pos + 1;
    const size_t limit    = std::min(text.size(), amp_pos + 1 + 32);  // 限制扫描范围
    if (i >= text.size()) {
        return false;
    }

    if (text[i] == '#') {
        ++i;
        bool hex = false;
        if (i < text.size() && (text[i] == 'x' || text[i] == 'X')) {
            hex = true;
            ++i;
        }
        const size_t digits_start = i;
        while (i < limit && i < text.size()) {
            const char ch = text[i];
            const bool ok = hex ? (is_digit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))
                                : is_digit(ch);
            if (!ok) {
                break;
            }
            ++i;
        }
        return i > digits_start && i < text.size() && text[i] == ';';
    }

    if (is_alpha(text[i])) {
        ++i;
        while (i < limit && i < text.size() && is_alnum(text[i])) {
            ++i;
        }
        return i < text.size() && text[i] == ';';
    }

    return false;
}

// 文本节点转义：转义 '<'、'>'，并对未构成字符引用的 '&' 转义为 &amp;。
void append_escaped_text(const std::string_view text, std::string& out) {
    for (size_t i = 0; i < text.size(); ++i) {
        switch (const char ch = text[i]) {
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '&':
                out += looks_like_char_reference(text, i) ? "&" : "&amp;";
                break;
            default:
                out += ch;
                break;
        }
    }
}

// 属性值转义（始终以双引号包裹）：转义 '"'，并对非字符引用的 '&' 转义。
void append_escaped_attribute(const std::string_view value, std::string& out) {
    for (size_t i = 0; i < value.size(); ++i) {
        switch (const char ch = value[i]) {
            case '"':
                out += "&quot;";
                break;
            case '&':
                out += looks_like_char_reference(value, i) ? "&" : "&amp;";
                break;
            default:
                out += ch;
                break;
        }
    }
}

void serialize_element(const Element& element, std::string& out) {
    const std::string& tag = element.tag_name();

    out += '<';
    out += tag;
    for (const auto& attr : element.attributes()) {
        out += ' ';
        out += attr.name();
        if (attr.has_value()) {
            out += "=\"";
            append_escaped_attribute(attr.value(), out);
            out += '"';
        }
    }
    out += '>';

    if (is_void_tag(tag)) {
        return;  // void 元素无内容与闭合标签
    }

    serialize_inner(element, out);

    out += "</";
    out += tag;
    out += '>';
}

}  // namespace

void serialize_children(const Node& node, std::string& out) {
    for (const Node* child = node.first_child(); child != nullptr; child = child->next_sibling()) {
        serialize_node(*child, out);
    }
}

void serialize_inner(const Element& element, std::string& out) {
    if (is_raw_text_tag(element.tag_name())) {
        for (const Node* child = element.first_child(); child != nullptr; child = child->next_sibling()) {
            if (const auto* text = child->as_text()) {
                out += text->value();  // raw text：原样输出，不转义
            }
        }
        return;
    }
    serialize_children(element, out);
}

void serialize_node(const Node& node, std::string& out) {
    if (const auto* element = node.as_element()) {
        serialize_element(*element, out);
        return;
    }
    if (const auto* text = node.as_text()) {
        append_escaped_text(text->value(), out);
        return;
    }
    if (const auto* comment = node.as_comment()) {
        out += "<!--";
        out += comment->value();
        out += "-->";
        return;
    }
    if (node.is_document()) {
        serialize_children(node, out);
    }
}

}  // namespace hps::detail
