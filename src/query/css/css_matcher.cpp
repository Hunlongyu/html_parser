#include "hps/query/css/css_matcher.hpp"

#include "hps/core/document.hpp"
#include "hps/core/element.hpp"

namespace hps {

std::vector<const Element*> CSSMatcher::find_all(const Element& element, const CSSSelector& selector) {
    std::vector<const Element*> results;
    for (auto child = element.first_child(); child; child = child->next_sibling()) {
        if (child->is_element()) {
            traverse_and_match(*child->as_element(), selector, results);
        }
    }
    return results;
}

std::vector<const Element*> CSSMatcher::find_all(const Element& element, const SelectorList& selector_list) {
    // 单次 DFS 中每个元素至多被 push 一次，结果天然按文档序且无重复，无需去重。
    std::vector<const Element*> results;
    for (auto child = element.first_child(); child; child = child->next_sibling()) {
        if (child->is_element()) {
            traverse_and_match(*child->as_element(), selector_list, results);
        }
    }
    return results;
}

std::vector<const Element*> CSSMatcher::find_all(const Document& document, const CSSSelector& selector) {
    std::vector<const Element*> results;
    for (auto child = document.first_child(); child; child = child->next_sibling()) {
        if (child->is_element()) {
            traverse_and_match(*child->as_element(), selector, results);
        }
    }
    return results;
}

std::vector<const Element*> CSSMatcher::find_all(const Document& document, const SelectorList& selector_list) {
    std::vector<const Element*> results;
    for (auto child = document.first_child(); child; child = child->next_sibling()) {
        if (child->is_element()) {
            traverse_and_match(*child->as_element(), selector_list, results);
        }
    }
    return results;
}

const Element* CSSMatcher::find_first(const Element& element, const CSSSelector& selector) {
    for (auto child = element.first_child(); child; child = child->next_sibling()) {
        if (child->is_element()) {
            auto element_child = child->as_element();
            if (selector.matches(*element_child)) {
                return element_child;
            }
            if (auto result = find_first(*element_child, selector)) {
                return result;
            }
        }
    }
    return nullptr;
}

const Element* CSSMatcher::find_first(const Element& element, const SelectorList& selector_list) {
    for (auto child = element.first_child(); child; child = child->next_sibling()) {
        if (child->is_element()) {
            auto element_child = child->as_element();
            if (selector_list.matches(*element_child)) {
                return element_child;
            }
            if (auto result = find_first(*element_child, selector_list)) {
                return result;
            }
        }
    }
    return nullptr;
}

const Element* CSSMatcher::find_first(const Document& document, const CSSSelector& selector) {
    for (auto child = document.first_child(); child; child = child->next_sibling()) {
        if (!child->is_element()) {
            continue;
        }

        const auto* element_child = child->as_element();
        if (selector.matches(*element_child)) {
            return element_child;
        }
        if (const auto* result = find_first(*element_child, selector)) {
            return result;
        }
    }
    return nullptr;
}

const Element* CSSMatcher::find_first(const Document& document, const SelectorList& selector_list) {
    for (auto child = document.first_child(); child; child = child->next_sibling()) {
        if (!child->is_element()) {
            continue;
        }

        const auto* element_child = child->as_element();
        if (selector_list.matches(*element_child)) {
            return element_child;
        }
        if (const auto* result = find_first(*element_child, selector_list)) {
            return result;
        }
    }
    return nullptr;
}

// ==================== DOM树遍历实现 ====================

void CSSMatcher::traverse_and_match(const Element& element, const CSSSelector& selector, std::vector<const Element*>& results) {
    // 检查当前元素
    if (selector.matches(element)) {
        results.push_back(&element);
    }
    // 递归遍历子元素
    for (auto child = element.first_child(); child; child = child->next_sibling()) {
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
    for (auto child = element.first_child(); child; child = child->next_sibling()) {
        if (child->is_element()) {
            traverse_and_match(*child->as_element(), selector_list, results);
        }
    }
}
}  // namespace hps
