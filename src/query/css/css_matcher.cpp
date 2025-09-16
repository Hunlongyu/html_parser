#include "hps/query/css/css_matcher.hpp"

#include "hps/core/document.hpp"
#include "hps/core/element.hpp"

#include <algorithm>
#include <regex>
#include <sstream>
#include <unordered_set>

namespace hps {

// ==================== 公共匹配接口实现 ====================

bool CSSMatcher::matches(const Element& element, const CSSSelector& selector) {
    // 直接使用选择器自身的matches方法，避免重复逻辑
    return selector.matches(element);
}

bool CSSMatcher::matches(const Element& element, const SelectorList& selector_list) {
    // 选择器列表采用"或"逻辑，只要匹配其中一个选择器即返回true
    for (const auto& selector : selector_list.selectors()) {
        if (matches(element, *selector)) {
            return true;
        }
    }
    return false;
}

// ==================== 元素查找接口实现 ====================

std::vector<std::shared_ptr<const Element>> CSSMatcher::find_all(const Element& root, const CSSSelector& selector) {
    std::vector<std::shared_ptr<const Element>> results;
    traverse_and_match(root, selector, results);
    return results;
}

std::vector<std::shared_ptr<const Element>> CSSMatcher::find_all(const Element& root, const SelectorList& selector_list) {
    std::vector<std::shared_ptr<const Element>> results;
    traverse_and_match(root, selector_list, results);

    // 去除重复元素并保持文档顺序
    std::unordered_set<const Element*> seen;
    auto                               it = std::ranges::remove_if(results, [&seen](const std::shared_ptr<const Element>& elem) { return !seen.insert(elem.get()).second; }).begin();
    results.erase(it, results.end());

    return results;
}

std::vector<std::shared_ptr<const Element>> CSSMatcher::find_all(const Document& document, const CSSSelector& selector) {
    auto root = document.root();
    if (!root) {
        return {};
    }
    return find_all(*root, selector);
}

std::vector<std::shared_ptr<const Element>> CSSMatcher::find_all(const Document& document, const SelectorList& selector_list) {
    const auto root = document.root();
    if (!root) {
        return {};
    }
    return find_all(*root, selector_list);
}

std::shared_ptr<const Element> CSSMatcher::find_first(const Element& root, const CSSSelector& selector) {
    // 检查根元素本身
    if (matches(root, selector)) {
        return root.shared_from_this()->as_element();
    }

    // 深度优先搜索子元素
    for (const auto& child : root.children()) {
        if (child->is_element()) {
            auto element_child = child->as_element();
            if (auto result = find_first(*element_child, selector)) {
                return result;
            }
        }
    }

    return nullptr;
}

std::shared_ptr<const Element> CSSMatcher::find_first(const Element& root, const SelectorList& selector_list) {
    // 检查根元素本身
    if (matches(root, selector_list)) {
        return root.shared_from_this()->as_element();
    }

    // 深度优先搜索子元素
    for (const auto& child : root.children()) {
        if (child->is_element()) {
            auto element_child = child->as_element();
            if (auto result = find_first(*element_child, selector_list)) {
                return result;
            }
        }
    }

    return nullptr;
}

std::shared_ptr<const Element> CSSMatcher::find_first(const Document& document, const CSSSelector& selector) {
    auto root = document.root();
    if (!root) {
        return nullptr;
    }
    return find_first(*root, selector);
}

std::shared_ptr<const Element> CSSMatcher::find_first(const Document& document, const SelectorList& selector_list) {
    auto root = document.root();
    if (!root) {
        return nullptr;
    }
    return find_first(*root, selector_list);
}

// ==================== DOM树遍历实现 ====================

void CSSMatcher::traverse_and_match(const Element& element, const CSSSelector& selector, std::vector<std::shared_ptr<const Element>>& results) {
    // 检查当前元素
    if (matches(element, selector)) {
        results.push_back(element.shared_from_this()->as_element());
    }

    // 递归遍历子元素
    for (const auto& child : element.children()) {
        if (child->is_element()) {
            traverse_and_match(*child->as_element(), selector, results);
        }
    }
}

void CSSMatcher::traverse_and_match(const Element& element, const SelectorList& selector_list, std::vector<std::shared_ptr<const Element>>& results) {
    // 检查当前元素
    if (matches(element, selector_list)) {
        results.push_back(element.shared_from_this()->as_element());
    }

    // 递归遍历子元素
    for (const auto& child : element.children()) {
        if (child->is_element()) {
            traverse_and_match(*child->as_element(), selector_list, results);
        }
    }
}

// ==================== 选择器类型匹配实现 ====================

bool CSSMatcher::matches_simple_selector(const Element& element, const CSSSelector& selector) {
    return selector.matches(element);
}

bool CSSMatcher::matches_compound_selector(const Element& element, const CompoundSelector& selector) {
    return selector.matches(element);
}

// ==================== 属性匹配实现 ====================

bool CSSMatcher::matches_attribute(const Element& element, const std::string& attr_name, AttributeOperator op, const std::string& value) {
    if (!element.has_attribute(attr_name)) {
        return false;
    }

    const std::string attr_value = element.get_attribute(attr_name);

    switch (op) {
        case AttributeOperator::Exists:
            return true;  // 属性存在即匹配

        case AttributeOperator::Equals:
            return attr_value == value;

        case AttributeOperator::Contains:
            return attr_value.find(value) != std::string::npos;

        case AttributeOperator::StartsWith:
            return attr_value.length() >= value.length() && attr_value.substr(0, value.length()) == value;

        case AttributeOperator::EndsWith:
            return attr_value.length() >= value.length() && attr_value.substr(attr_value.length() - value.length()) == value;

        case AttributeOperator::WordMatch: {
            // 单词匹配：属性值包含指定单词（以空白字符分隔）
            std::istringstream iss(attr_value);
            std::string        word;
            while (iss >> word) {
                if (word == value) {
                    return true;
                }
            }
            return false;
        }

        case AttributeOperator::LangMatch:
            // 语言匹配：属性值等于指定值或以"指定值-"开头
            return attr_value == value || (attr_value.length() > value.length() && attr_value.substr(0, value.length() + 1) == value + "-");

        default:
            return false;
    }
}

// ==================== DOM导航辅助实现 ====================

std::shared_ptr<const Element> CSSMatcher::get_parent_element(const Element& element) {
    auto parent = element.parent();
    if (parent && parent->is_element()) {
        return parent->as_element();
    }
    return nullptr;
}

std::vector<std::shared_ptr<const Element>> CSSMatcher::get_ancestor_elements(const Element& element) {
    std::vector<std::shared_ptr<const Element>> ancestors;

    auto current = element.parent();
    while (current) {
        if (current->is_element()) {
            ancestors.push_back(current->as_element());
        }
        current = current->parent();
    }

    return ancestors;
}

std::shared_ptr<const Element> CSSMatcher::get_previous_sibling_element(const Element& element) {
    auto parent = element.parent();
    if (!parent) {
        return nullptr;
    }

    const auto& siblings = parent->children();
    auto        it       = std::ranges::find_if(siblings, [&element](const std::shared_ptr<const Node>& node) { return node.get() == &element; });

    if (it == siblings.begin()) {
        return nullptr;  // 没有前面的兄弟节点
    }

    // 向前查找最近的元素节点
    auto prev_it = it;
    while (prev_it != siblings.begin()) {
        --prev_it;
        if ((*prev_it)->is_element()) {
            return (*prev_it)->as_element();
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<const Element>> CSSMatcher::get_previous_sibling_elements(const Element& element) {
    std::vector<std::shared_ptr<const Element>> siblings;

    auto parent = element.parent();
    if (!parent) {
        return siblings;
    }

    const auto& all_siblings = parent->children();
    auto        it           = std::ranges::find_if(all_siblings, [&element](const std::shared_ptr<const Node>& node) { return node.get() == &element; });

    if (it == all_siblings.begin()) {
        return siblings;  // 没有前面的兄弟节点
    }

    // 收集所有前面的元素节点（按从近到远的顺序）
    auto prev_it = it;
    while (prev_it != all_siblings.begin()) {
        --prev_it;
        if ((*prev_it)->is_element()) {
            siblings.insert(siblings.begin(), (*prev_it)->as_element());
        }
    }

    return siblings;
}

}  // namespace hps