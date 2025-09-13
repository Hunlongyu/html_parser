#pragma once
#include "string_view"

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

inline const std::unordered_set<std::string_view>& get_void_elements() {
    static const std::unordered_set<std::string_view> void_elements = {"area", "base", "br", "col", "embed", "hr", "img", "input", "link", "meta", "param", "source", "track", "wbr"};
    return void_elements;
}

inline const std::unordered_set<std::string_view>& get_raw_text_elements() {
    static const std::unordered_set<std::string_view> raw_text_elements = {"script", "style", "textarea", "title"};
    return raw_text_elements;
}

inline bool is_void_element(const std::string_view tag_name) noexcept {
    return get_void_elements().contains(tag_name);
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

}  // namespace hps