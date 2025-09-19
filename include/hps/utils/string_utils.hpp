#pragma once
#include "string_view"

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <iconv.h>
#endif

#include <stdexcept>
#include <unordered_set>

namespace hps {

inline bool is_whitespace(const char c) {
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

// 添加安全的字符类型检查函数
inline bool safe_isalnum(const char ch) noexcept {
    return static_cast<unsigned char>(ch) <= 255 && std::isalnum(static_cast<unsigned char>(ch));
}

inline bool safe_isalpha(const char ch) noexcept {
    return static_cast<unsigned char>(ch) <= 255 && std::isalpha(static_cast<unsigned char>(ch));
}

inline bool safe_isdigit(const char ch) noexcept {
    return static_cast<unsigned char>(ch) <= 255 && std::isdigit(static_cast<unsigned char>(ch));
}

inline bool safe_isspace(const char ch) noexcept {
    return static_cast<unsigned char>(ch) <= 255 && std::isspace(static_cast<unsigned char>(ch));
}

inline char safe_tolower(const char ch) noexcept {
    return static_cast<unsigned char>(ch) <= 255 ? std::tolower(static_cast<unsigned char>(ch)) : ch;
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

inline bool is_valid_identifier_start(const std::string& input, const size_t pos) {
    if (pos >= input.size())
        return false;

    const unsigned char c = static_cast<unsigned char>(input[pos]);

    // ASCII字母、下划线、连字符
    if (std::isalpha(c) || c == '_' || c == '-') {
        return true;
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

inline bool is_valid_identifier_char(const std::string& input, const size_t pos) {
    if (pos >= input.size())
        return false;

    const unsigned char c = static_cast<unsigned char>(input[pos]);

    // ASCII字母、数字、下划线、连字符
    if (std::isalnum(c) || c == '_' || c == '-') {
        return true;
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

}  // namespace hps