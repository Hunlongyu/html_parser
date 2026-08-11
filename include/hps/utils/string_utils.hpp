#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace hps {

// ==================== 轻量字符级助手（内联，热路径） ====================

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

// ==================== UTF-8 助手（内联） ====================

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
    const int len = utf8_char_length(c);
    if (pos + len <= input.size()) {
        for (int i = 1; i < len; ++i) {
            if (pos + i >= input.size() || (static_cast<unsigned char>(input[pos + i]) & 0xC0) != 0x80) {
                return false;
            }
        }
        return true;
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
    const int len = utf8_char_length(c);
    if (pos + len <= input.size()) {
        for (int i = 1; i < len; ++i) {
            if (pos + i >= input.size() || (static_cast<unsigned char>(input[pos + i]) & 0xC0) != 0x80) {
                return false;
            }
        }
        return true;
    }
    return false;
}

// ==================== 较重的工具（实现见 string_utils.cpp） ====================

/**
 * @brief 解码 HTML 实体（命名实体与数字实体）
 * @param text 含 HTML 实体的文本
 * @return 解码后的文本
 */
[[nodiscard]] std::string decode_html_entities(const std::string& text);

/**
 * @brief 按 HTML 属性值上下文解码字符引用
 *
 * 与文本上下文的区别是：旧式无分号命名引用后紧跟 ASCII 字母数字或 '=' 时保持原样。
 */
[[nodiscard]] std::string decode_html_attribute_entities(std::string_view text);

/**
 * @brief 标准化空白（连续空白合并为单个空格）
 * @param text 待处理文本
 * @return 标准化后的文本
 */
[[nodiscard]] std::string normalize_whitespace(const std::string& text);

}  // namespace hps
