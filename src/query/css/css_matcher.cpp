#include "hps/query/css/css_matcher.hpp"

#include "hps/core/document.hpp"
#include "hps/core/element.hpp"

#include <algorithm>
#include <regex>
#include <unordered_set>

namespace hps {

std::vector<const Element*> CSSMatcher::find_all(const Element& element, const CSSSelector& selector) {
    std::vector<const Element*> results;
    traverse_and_match(element, selector, results);
    return results;
}

std::vector<const Element*> CSSMatcher::find_all(const Element& element, const SelectorList& selector_list) {
    std::vector<const Element*> results;
    traverse_and_match(element, selector_list, results);
    std::unordered_set<const Element*> seen;
    const auto                         it = std::ranges::remove_if(results, [&seen](const Element* elem) { return !seen.insert(elem).second; }).begin();
    results.erase(it, results.end());

    return results;
}

std::vector<const Element*> CSSMatcher::find_all(const Document& document, const CSSSelector& selector) {
    const auto root = document.root();
    if (!root) {
        return {};
    }
    return find_all(*root, selector);
}

std::vector<const Element*> CSSMatcher::find_all(const Document& document, const SelectorList& selector_list) {
    const auto root = document.root();
    if (!root) {
        return {};
    }
    return find_all(*root, selector_list);
}

const Element* CSSMatcher::find_first(const Element& element, const CSSSelector& selector) {
    // 检查根元素本身
    if (selector.matches(element)) {
        return &element;
    }
    // 深度优先搜索子元素
    for (const auto& child : element.children()) {
        if (child->is_element()) {
            auto element_child = child->as_element();
            if (auto result = find_first(*element_child, selector)) {
                return result;
            }
        }
    }
    return nullptr;
}

const Element* CSSMatcher::find_first(const Element& element, const SelectorList& selector_list) {
    // 检查根元素本身
    if (selector_list.matches(element)) {
        return &element;
    }
    // 深度优先搜索子元素
    for (const auto& child : element.children()) {
        if (child->is_element()) {
            auto element_child = child->as_element();
            if (auto result = find_first(*element_child, selector_list)) {
                return result;
            }
        }
    }
    return nullptr;
}

const Element* CSSMatcher::find_first(const Document& document, const CSSSelector& selector) {
    const auto root = document.root();
    if (!root) {
        return nullptr;
    }
    return find_first(*root, selector);
}

const Element* CSSMatcher::find_first(const Document& document, const SelectorList& selector_list) {
    const auto root = document.root();
    if (!root) {
        return nullptr;
    }
    return find_first(*root, selector_list);
}

// ==================== DOM树遍历实现 ====================

void CSSMatcher::traverse_and_match(const Element& element, const CSSSelector& selector, std::vector<const Element*>& results) {
    // 检查当前元素
    if (selector.matches(element)) {
        results.push_back(&element);
    }
    // 递归遍历子元素
    for (const auto& child : element.children()) {
        if (child->is_element()) {
            traverse_and_match(*child->as_element(), selector, results);
        }
    }
}

void CSSMatcher::traverse_and_match(const Element& element, const SelectorList& selector_list, std::vector<const Element*>& results) {
    // 检查当前元素
    if (selector_list.matches(element)) {
        results.push_back(&element);
    }
    // 递归遍历子元素
    for (const auto& child : element.children()) {
        if (child->is_element()) {
            traverse_and_match(*child->as_element(), selector_list, results);
        }
    }
}
}  // namespace hps
