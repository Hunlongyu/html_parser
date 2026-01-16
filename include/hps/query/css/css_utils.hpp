#pragma once

#include "css_parser.hpp"
#include "css_selector.hpp"
#include "hps/utils/string_utils.hpp"

#include <memory>
#include <string>
#include <vector>

namespace hps {

inline bool is_valid_selector(const std::string_view selector) {
    CSSParser parser(selector);
    return parser.validate();
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
        } else if (is_whitespace(c)) {
            if (!last_was_space && !result.empty()) {
                result += ' ';
                last_was_space = true;
            }
        } else {
            result += to_lower(c);
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
    // CSSParser handles normalization (skipping whitespace, lowercasing types/attributes)
    // We pass the raw selector to avoid unnecessary string copy.
    CSSParser parser(selector, options);
    auto result = parser.parse_selector_list();
    // In strict mode, parser would have thrown.
    // In lenient mode, we return what we have (even if partial).
    return result;
}

inline std::unique_ptr<SelectorList> parse_css_selector(const std::string_view selector) {
    return parse_css_selector(selector, Options{});
}

inline std::vector<std::unique_ptr<SelectorList>> parse_css_selectors(const std::vector<std::string_view>& selectors) {
    std::vector<std::unique_ptr<SelectorList>> results;
    results.reserve(selectors.size());

    for (const auto& selector : selectors) {
        auto result = parse_css_selector(selector);
        results.push_back(std::move(result));
    }

    return results;
}
}  // namespace hps
