#pragma once

#include "css_parser.hpp"
#include "css_selector.hpp"
#include "hps/utils/string_utils.hpp"

#include <memory>
#include <list>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

namespace hps {

enum class SimpleSelectorKind : std::uint8_t {
    Type,
    Class,
    Id
};

struct SimpleSelectorView {
    SimpleSelectorKind kind;
    std::string_view   value;
};

inline bool is_simple_selector_identifier(const std::string_view identifier) {
    if (identifier.empty() || !is_valid_identifier_start(identifier, 0)) {
        return false;
    }

    for (size_t i = 0; i < identifier.size();) {
        if (!is_valid_identifier_char(identifier, i)) {
            return false;
        }

        const unsigned char ch = static_cast<unsigned char>(identifier[i]);
        i += ch < 0x80 ? 1U : static_cast<size_t>(utf8_char_length(ch));
    }

    return true;
}

inline std::optional<SimpleSelectorView> classify_simple_selector(std::string_view selector) {
    selector = trim_whitespace(selector);
    if (selector.empty()) {
        return std::nullopt;
    }

    if (selector.front() == '#') {
        const auto id = selector.substr(1);
        if (is_simple_selector_identifier(id)) {
            return SimpleSelectorView{SimpleSelectorKind::Id, id};
        }
        return std::nullopt;
    }

    if (selector.front() == '.') {
        const auto class_name = selector.substr(1);
        if (is_simple_selector_identifier(class_name)) {
            return SimpleSelectorView{SimpleSelectorKind::Class, class_name};
        }
        return std::nullopt;
    }

    if (is_simple_selector_identifier(selector)) {
        return SimpleSelectorView{SimpleSelectorKind::Type, selector};
    }

    return std::nullopt;
}

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

inline std::shared_ptr<const SelectorList> parse_css_selector_cached(const std::string_view selector, const Options& options = {}) {
    if (options.max_css3_cache_size == 0) {
        auto parsed = parse_css_selector(selector, options);
        return std::shared_ptr<const SelectorList>(parsed.release());
    }

    struct SelectorCacheEntry {
        std::string                      key;
        std::shared_ptr<const SelectorList> selector_list;
    };

    thread_local std::list<SelectorCacheEntry> cache_entries;
    thread_local std::unordered_map<std::string, std::list<SelectorCacheEntry>::iterator> cache_index;

    const std::string cache_key = std::to_string(static_cast<int>(options.error_handling)) + '\n' + std::string(selector);
    if (const auto it = cache_index.find(cache_key); it != cache_index.end()) {
        cache_entries.splice(cache_entries.begin(), cache_entries, it->second);
        return it->second->selector_list;
    }

    auto parsed = parse_css_selector(selector, options);
    auto cached_selector = std::shared_ptr<const SelectorList>(parsed.release());

    cache_entries.push_front(SelectorCacheEntry{cache_key, cached_selector});
    cache_index[cache_entries.front().key] = cache_entries.begin();

    while (cache_entries.size() > options.max_css3_cache_size) {
        auto last = std::prev(cache_entries.end());
        cache_index.erase(last->key);
        cache_entries.pop_back();
    }

    return cached_selector;
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
