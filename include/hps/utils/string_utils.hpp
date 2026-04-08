#pragma once
#include <string_view>

#include <array>
#include <cstdint>
#include <optional>
#include <regex>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <iconv.h>
#endif

#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace hps {

inline bool is_letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool is_whitespace(const char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
}

inline bool is_alpha(const char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool is_alnum(const char c) noexcept {
    return is_alpha(c) || (c >= '0' && c <= '9');
}

inline bool is_digit(const char c) noexcept {
    return c >= '0' && c <= '9';
}

inline bool is_hex_digit(const char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}
inline char to_lower(const char c) noexcept {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c - 'A' + 'a');
    }
    return c;
}

inline char to_upper(const char c) noexcept {
    if (c >= 'a' && c <= 'z') {
        return static_cast<char>(c - 'a' + 'A');
    }
    return c;
}

inline std::string_view trim_whitespace(std::string_view str) noexcept {
    while (!str.empty() && is_whitespace(str.front())) {
        str.remove_prefix(1);
    }
    while (!str.empty() && is_whitespace(str.back())) {
        str.remove_suffix(1);
    }
    return str;
}

inline bool starts_with_ignore_case(std::string_view str, std::string_view prefix) noexcept {
    if (str.length() < prefix.length()) {
        return false;
    }
    for (size_t i = 0; i < prefix.length(); ++i) {
        if (to_lower(str[i]) != to_lower(prefix[i])) {
            return false;
        }
    }
    return true;
}

inline bool equals_ignore_case(std::string_view a, std::string_view b) noexcept {
    if (a.length() != b.length()) {
        return false;
    }
    for (size_t i = 0; i < a.length(); ++i) {
        if (to_lower(a[i]) != to_lower(b[i])) {
            return false;
        }
    }
    return true;
}

inline const std::unordered_set<std::string_view>& get_raw_text_elements() {
    static const std::unordered_set<std::string_view> raw_text_elements = {"script", "style", "textarea", "title"};
    return raw_text_elements;
}

inline bool is_raw_text_element(const std::string_view tag_name) noexcept {
    return get_raw_text_elements().contains(tag_name);
}

/**
 * @brief 解析 class 属性值，提取所有类名
 * @param class_attr class 属性的值
 * @return 包含所有类名的无序集合
 */
inline std::unordered_set<std::string> split_class_names(const std::string_view class_attr) {
    std::unordered_set<std::string> class_names;
    size_t                          start = 0;
    while (start < class_attr.length()) {
        while (start < class_attr.length() && is_whitespace(class_attr[start])) {
            ++start;
        }
        if (start >= class_attr.length()) {
            break;
        }
        size_t end = start;
        while (end < class_attr.length() && !is_whitespace(class_attr[end])) {
            ++end;
        }
        if (end > start) {
            class_names.emplace(class_attr.substr(start, end - start));
        }
        start = end;
    }
    return class_names;
}

// UTF-8 辅助函数
inline bool is_utf8_start_byte(const unsigned char c) {
    return (c & 0x80) == 0 || (c & 0xE0) == 0xC0 || (c & 0xF0) == 0xE0 || (c & 0xF8) == 0xF0;
}

inline int utf8_char_length(const unsigned char c) {
    if ((c & 0x80) == 0)
        return 1;  // 0xxxxxxx - ASCII
    if ((c & 0xE0) == 0xC0)
        return 2;  // 110xxxxx - 2字节UTF-8
    if ((c & 0xF0) == 0xE0)
        return 3;  // 1110xxxx - 3字节UTF-8
    if ((c & 0xF8) == 0xF0)
        return 4;  // 11110xxx - 4字节UTF-8
    return 1;      // 无效字节，按1字节处理
}

inline bool is_valid_identifier_start(std::string_view input, const size_t pos) {
    if (pos >= input.size())
        return false;

    const unsigned char c = static_cast<unsigned char>(input[pos]);

    // ASCII字母、下划线、连字符
    if (c < 0x80) {
        return is_alpha(static_cast<char>(c)) || c == '_' || c == '-';
    }

    // UTF-8多字节字符（中文等）
    if (c >= 0x80) {
        const int len = utf8_char_length(c);
        if (pos + len <= input.size()) {
            // 简单检查：如果是多字节UTF-8序列，认为是有效的标识符字符
            bool valid_sequence = true;
            for (int i = 1; i < len; ++i) {
                if (pos + i >= input.size() || (static_cast<unsigned char>(input[pos + i]) & 0xC0) != 0x80) {
                    valid_sequence = false;
                    break;
                }
            }
            return valid_sequence;
        }
    }

    return false;
}

inline bool is_valid_identifier_char(std::string_view input, const size_t pos) {
    if (pos >= input.size())
        return false;

    const unsigned char c = static_cast<unsigned char>(input[pos]);

    // ASCII字母、数字、下划线、连字符
    if (c < 0x80) {
        return is_alnum(static_cast<char>(c)) || c == '_' || c == '-';
    }

    // UTF-8多字节字符
    if (c >= 0x80) {
        const int len = utf8_char_length(c);
        if (pos + len <= input.size()) {
            bool valid_sequence = true;
            for (int i = 1; i < len; ++i) {
                if (pos + i >= input.size() || (static_cast<unsigned char>(input[pos + i]) & 0xC0) != 0x80) {
                    valid_sequence = false;
                    break;
                }
            }
            return valid_sequence;
        }
    }

    return false;
}

inline std::string gbk_to_utf8(std::string_view gbk_str) {
    if (gbk_str.empty()) {
        return {};
    }
#ifdef _WIN32
    const int wide_size = MultiByteToWideChar(936, 0, gbk_str.data(), static_cast<int>(gbk_str.size()), nullptr, 0);
    if (wide_size <= 0) {
        throw std::runtime_error("Failed to calculate UTF-16 size");
    }
    std::vector<wchar_t> wide_str(wide_size);
    if (MultiByteToWideChar(936, 0, gbk_str.data(), static_cast<int>(gbk_str.size()), wide_str.data(), wide_size) != wide_size) {
        throw std::runtime_error("Failed to convert GBK to UTF-16");
    }
    const int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wide_str.data(), wide_size, nullptr, 0, nullptr, nullptr);
    if (utf8_size <= 0) {
        throw std::runtime_error("Failed to calculate UTF-8 size");
    }
    std::string utf8_str(utf8_size, '\0');
    if (WideCharToMultiByte(CP_UTF8, 0, wide_str.data(), wide_size, utf8_str.data(), utf8_size, nullptr, nullptr) != utf8_size) {
        throw std::runtime_error("Failed to convert UTF-16 to UTF-8");
    }
    return utf8_str;

#else
    // Linux/macOS 实现 (使用 iconv)
    iconv_t cd = iconv_open("UTF-8", "GBK");
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        throw std::runtime_error("Failed to open iconv for GBK to UTF-8 conversion");
    }
    size_t            in_bytes_left  = gbk_str.size();
    size_t            out_bytes_left = gbk_str.size() * 4;  // UTF-8 最多4字节
    std::vector<char> out_buffer(out_bytes_left);
    char*             in_ptr  = const_cast<char*>(gbk_str.data());
    char*             out_ptr = out_buffer.data();
    size_t            result  = iconv(cd, &in_ptr, &in_bytes_left, &out_ptr, &out_bytes_left);
    iconv_close(cd);
    if (result == static_cast<size_t>(-1)) {
        throw std::runtime_error("Failed to convert GBK to UTF-8");
    }
    return std::string(out_buffer.data(), out_buffer.size() - out_bytes_left);
#endif
}

/**
 * @brief 解码HTML实体
 *
 * @param text 包含HTML实体的文本
 * @return 解码后的文本
 */
inline std::string decode_html_entities(const std::string& text) {
    static constexpr std::array<std::pair<std::string_view, std::string_view>, 26> named_entities = {{
        {"amp", "&"},
        {"lt", "<"},
        {"gt", ">"},
        {"quot", "\""},
        {"apos", "'"},
        {"nbsp", " "},
        {"copy", "\xC2\xA9"},
        {"reg", "\xC2\xAE"},
        {"trade", "\xE2\x84\xA2"},
        {"mdash", "\xE2\x80\x94"},
        {"ndash", "\xE2\x80\x93"},
        {"hellip", "\xE2\x80\xA6"},
        {"middot", "\xC2\xB7"},
        {"bull", "\xE2\x80\xA2"},
        {"euro", "\xE2\x82\xAC"},
        {"laquo", "\xC2\xAB"},
        {"raquo", "\xC2\xBB"},
        {"iexcl", "\xC2\xA1"},
        {"cent", "\xC2\xA2"},
        {"pound", "\xC2\xA3"},
        {"yen", "\xC2\xA5"},
        {"sect", "\xC2\xA7"},
        {"para", "\xC2\xB6"},
        {"deg", "\xC2\xB0"},
        {"times", "\xC3\x97"},
        {"divide", "\xC3\xB7"},
    }};

    const auto append_code_point_utf8 = [](std::string& output, const uint32_t code_point) {
        if (code_point <= 0x7F) {
            output.push_back(static_cast<char>(code_point));
            return;
        }
        if (code_point <= 0x7FF) {
            output.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
            output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
            return;
        }
        if (code_point <= 0xFFFF) {
            output.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
            output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
            return;
        }
        output.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
        output.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    };

    const auto parse_numeric_entity = [](const std::string_view entity_body) -> std::optional<uint32_t> {
        if (entity_body.size() < 2 || entity_body.front() != '#') {
            return std::nullopt;
        }

        uint32_t       value    = 0;
        size_t         index    = 1;
        const unsigned base     = (index < entity_body.size() && (entity_body[index] == 'x' || entity_body[index] == 'X')) ? 16U : 10U;
        if (base == 16U) {
            ++index;
        }
        if (index >= entity_body.size()) {
            return std::nullopt;
        }

        for (; index < entity_body.size(); ++index) {
            const char ch = entity_body[index];
            if (base == 16U) {
                if (!is_hex_digit(ch)) {
                    return std::nullopt;
                }
                value = static_cast<uint32_t>(value * 16 + (is_digit(ch) ? (ch - '0') : (to_lower(ch) - 'a' + 10)));
            } else {
                if (!is_digit(ch)) {
                    return std::nullopt;
                }
                value = static_cast<uint32_t>(value * 10 + (ch - '0'));
            }
            if (value > 0x10FFFF) {
                return std::nullopt;
            }
        }

        if (value == 0 || (value >= 0xD800 && value <= 0xDFFF)) {
            return std::nullopt;
        }
        return value;
    };

    const std::string_view input(text);
    std::string            out;
    out.reserve(text.size());

    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '&') {
            const size_t semicolon = input.find(';', i + 1);
            if (semicolon != std::string_view::npos) {
                const std::string_view entity_body = input.substr(i + 1, semicolon - i - 1);

                if (const auto code_point = parse_numeric_entity(entity_body); code_point.has_value()) {
                    append_code_point_utf8(out, *code_point);
                    i = semicolon + 1;
                    continue;
                }

                bool matched_named_entity = false;
                for (const auto& [name, replacement] : named_entities) {
                    if (entity_body == name) {
                        out.append(replacement);
                        i = semicolon + 1;
                        matched_named_entity = true;
                        break;
                    }
                }

                if (matched_named_entity) {
                    continue;
                }
            }
        }
        out.push_back(input[i]);
        ++i;
    }

    return out;
}

/**
 * @brief 标准化空白字符（合并连续空白为单个空格）
 * @param text 要处理的文本
 * @return 标准化后的文本
 */
inline std::string normalize_whitespace(const std::string& text) {
    std::string result;
    result.reserve(text.length());

    bool in_whitespace = false;
    for (char c : text) {
        if (is_whitespace(c)) {
            if (!in_whitespace) {
                result += ' ';  // 用单个空格替换所有空白字符
                in_whitespace = true;
            }
            // 跳过连续的空白字符
        } else {
            result += c;
            in_whitespace = false;
        }
    }

    return result;
}
}  // namespace hps
