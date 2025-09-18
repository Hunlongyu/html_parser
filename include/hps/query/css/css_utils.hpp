#pragma once

#include "css_parser.hpp"
#include "css_selector.hpp"
#include "hps/utils/string_utils.hpp"

#include <memory>
#include <string>
#include <vector>

namespace hps {

inline bool is_valid_selector(const std::string_view selector) {
    try {
        CSSParser  parser(selector);
        const auto result = parser.parse_selector_list();
        return result && !result->empty() && !parser.has_errors();
    } catch (const HPSException&) {
        return false;
    }
}

inline std::string normalize_selector(const std::string_view selector) {
    // 移除多余空格，统一格式
    std::string result;
    result.reserve(selector.size());

    bool in_string      = false;
    char string_quote   = '\0';
    bool last_was_space = false;

    for (const char c : selector) {
        if (!in_string && (c == '"' || c == '\'')) {
            in_string    = true;
            string_quote = c;
            result += c;
            last_was_space = false;
        } else if (in_string && c == string_quote) {
            in_string    = false;
            string_quote = '\0';
            result += c;
            last_was_space = false;
        } else if (in_string) {
            result += c;
            last_was_space = false;
        } else if (safe_isspace(c)) {
            if (!last_was_space && !result.empty()) {
                result += ' ';
                last_was_space = true;
            }
        } else {
            result += safe_tolower(c);
            last_was_space = false;
        }
    }

    // 移除尾部空格
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }

    return result;
}

inline std::unique_ptr<SelectorList> parse_css_selector(const std::string_view selector, const Options& options) {
    const std::string normalized = normalize_selector(selector);
    CSSParser         parser(normalized, options);  // 使用规范化后的选择器字符串
    auto              result = parser.parse_selector_list();
    if (parser.has_errors() && options.error_handling == ErrorHandlingMode::Strict) {
        throw HPSException(ErrorCode::InvalidSelector, "CSS parsing failed");
    }
    return result;
}

inline std::unique_ptr<SelectorList> parse_css_selector(const std::string_view selector) {
    return parse_css_selector(selector, Options{});
}

inline std::vector<std::unique_ptr<SelectorList>> parse_css_selectors(const std::vector<std::string_view>& selectors) {
    std::vector<std::unique_ptr<SelectorList>> results;
    results.reserve(selectors.size());

    for (const auto& selector : selectors) {
        try {
            auto result = parse_css_selector(selector);
            results.push_back(std::move(result));
        } catch (const HPSException&) {
            results.push_back(nullptr);
        }
    }

    return results;
}
}  // namespace hps