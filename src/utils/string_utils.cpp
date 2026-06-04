#include "hps/utils/string_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

// WHATWG 具名字符引用表（自动生成，约 2231 条，含旧式无分号形式）。
#include "html_entities.inc"

namespace hps {

namespace {

// HTML5「numeric character reference end state」的 C1（0x80–0x9F）→ Windows-1252 重映射；
// 未列出的 5 个码位（0x81/0x8D/0x8F/0x90/0x9D）保持原值。非法码位归一为 U+FFFD。
[[nodiscard]] uint32_t remap_numeric_code_point(const uint32_t code_point) {
    if (code_point == 0 || code_point > 0x10FFFF || (code_point >= 0xD800 && code_point <= 0xDFFF)) {
        return 0xFFFD;
    }
    switch (code_point) {
        case 0x80: return 0x20AC;
        case 0x82: return 0x201A;
        case 0x83: return 0x0192;
        case 0x84: return 0x201E;
        case 0x85: return 0x2026;
        case 0x86: return 0x2020;
        case 0x87: return 0x2021;
        case 0x88: return 0x02C6;
        case 0x89: return 0x2030;
        case 0x8A: return 0x0160;
        case 0x8B: return 0x2039;
        case 0x8C: return 0x0152;
        case 0x8E: return 0x017D;
        case 0x91: return 0x2018;
        case 0x92: return 0x2019;
        case 0x93: return 0x201C;
        case 0x94: return 0x201D;
        case 0x95: return 0x2022;
        case 0x96: return 0x2013;
        case 0x97: return 0x2014;
        case 0x98: return 0x02DC;
        case 0x99: return 0x2122;
        case 0x9A: return 0x0161;
        case 0x9B: return 0x203A;
        case 0x9C: return 0x0153;
        case 0x9E: return 0x017E;
        case 0x9F: return 0x0178;
        default:   return code_point;
    }
}

void append_code_point_utf8(std::string& output, const uint32_t code_point) {
    if (code_point <= 0x7F) {
        output.push_back(static_cast<char>(code_point));
    } else if (code_point <= 0x7FF) {
        output.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
        output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    } else if (code_point <= 0xFFFF) {
        output.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
        output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    } else {
        output.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
        output.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    }
}

}  // namespace

// 解码文本中的 HTML 字符引用（data / RCDATA 上下文）。属性值不走此函数。
std::string decode_html_entities(const std::string& text) {
    static const std::unordered_map<std::string_view, std::string_view> entity_map = [] {
        std::unordered_map<std::string_view, std::string_view> map;
        map.reserve(detail::kNamedEntityCount * 2);
        for (const auto& [name, value] : detail::kNamedEntities) {
            map.emplace(name, value);
        }
        return map;
    }();

    const std::string_view input(text);
    std::string            out;
    out.reserve(text.size());

    size_t i = 0;
    while (i < input.size()) {
        if (input[i] != '&') {
            out.push_back(input[i]);
            ++i;
            continue;
        }

        // 数值字符引用：&#123; / &#xAF;（分号可省略）。
        if (i + 1 < input.size() && input[i + 1] == '#') {
            size_t     j   = i + 2;
            const bool hex = j < input.size() && (input[j] == 'x' || input[j] == 'X');
            if (hex) {
                ++j;
            }
            const size_t digits_start = j;
            uint32_t     value        = 0;
            for (; j < input.size(); ++j) {
                const char ch = input[j];
                unsigned   digit;
                if (hex && is_hex_digit(ch)) {
                    digit = is_digit(ch) ? static_cast<unsigned>(ch - '0')
                                         : static_cast<unsigned>(to_lower(ch) - 'a' + 10);
                } else if (!hex && is_digit(ch)) {
                    digit = static_cast<unsigned>(ch - '0');
                } else {
                    break;
                }
                if (value <= 0x10FFFF) {  // 溢出后冻结为越界值，最终归一为 U+FFFD
                    value = value * (hex ? 16U : 10U) + digit;
                }
            }
            if (j == digits_start) {
                // "&#" 后无数字：& 原样输出，从 # 继续。
                out.push_back('&');
                ++i;
                continue;
            }
            if (j < input.size() && input[j] == ';') {
                ++j;
            }
            append_code_point_utf8(out, remap_numeric_code_point(value));
            i = j;
            continue;
        }

        // 具名字符引用：从 & 之后做最长前缀匹配（表内已含旧式无分号形式，
        // 故 &gt 与 &gt; 均可解，&notin; 优先于 &not）。
        if (i + 1 < input.size() && is_alnum(input[i + 1])) {
            const size_t max_len = std::min(detail::kMaxNamedEntityLength, input.size() - (i + 1));
            bool         matched = false;
            for (size_t len = max_len; len >= 1; --len) {
                const auto it = entity_map.find(input.substr(i + 1, len));
                if (it != entity_map.end()) {
                    out.append(it->second);
                    i += 1 + len;
                    matched = true;
                    break;
                }
            }
            if (matched) {
                continue;
            }
        }

        out.push_back('&');
        ++i;
    }

    return out;
}

std::string normalize_whitespace(const std::string& text) {
    std::string result;
    result.reserve(text.length());

    bool in_whitespace = false;
    for (char c : text) {
        if (is_whitespace(c)) {
            if (!in_whitespace) {
                result += ' ';  // 用单个空格替换所有连续空白
                in_whitespace = true;
            }
        } else {
            result += c;
            in_whitespace = false;
        }
    }

    return result;
}

}  // namespace hps
