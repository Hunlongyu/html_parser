#include "hps/query/css/css_selector.hpp"

#include "hps/core/element.hpp"
#include "hps/utils/string_utils.hpp"

#include <algorithm>

namespace hps {

// ==================== TypeSelector Implementation ====================

bool TypeSelector::matches(const Element& element) const {
    return equals_ignore_case(element.tag_name(), m_tag_name);
}

bool TypeSelector::can_quick_reject(const Element& element) const {
    return !equals_ignore_case(element.tag_name(), m_tag_name);
}

// ==================== ClassSelector Implementation ====================

bool ClassSelector::matches(const Element& element) const {
    return element.has_class(m_class_name);
}

bool ClassSelector::can_quick_reject(const Element& element) const {
    return !element.has_attribute("class");
}

// ==================== IdSelector Implementation ====================

bool IdSelector::matches(const Element& element) const {
    return element.has_attribute("id") && element.get_attribute("id") == m_id_name;
}

bool IdSelector::can_quick_reject(const Element& element) const {
    return !element.has_attribute("id") || element.get_attribute("id") != m_id_name;
}

// ==================== AttributeSelector Implementation ====================

bool AttributeSelector::matches(const Element& element) const {
    if (!element.has_attribute(m_attr_name)) {
        return false;
    }

    if (m_operator == AttributeOperator::Exists) {
        return true;
    }

    const auto& attr_value = element.get_attribute(m_attr_name);
    return matches_attribute_value(attr_value);
}

std::string AttributeSelector::to_string() const {
    std::string result = "[";
    result += m_attr_name;

    switch (m_operator) {
        case AttributeOperator::Exists:
            break;
        case AttributeOperator::Equals:
            result += "=\"";
            result += m_value;
            result += "\"";
            break;
        case AttributeOperator::Contains:
            result += "*=\"";
            result += m_value;
            result += "\"";
            break;
        case AttributeOperator::StartsWith:
            result += "^=\"";
            result += m_value;
            result += "\"";
            break;
        case AttributeOperator::EndsWith:
            result += "$=\"";
            result += m_value;
            result += "\"";
            break;
        case AttributeOperator::WordMatch:
            result += "~=\"";
            result += m_value;
            result += "\"";
            break;
        case AttributeOperator::LangMatch:
            result += "|=\"";
            result += m_value;
            result += "\"";
            break;
    }

    result += "]";
    return result;
}

bool AttributeSelector::matches_attribute_value(const std::string_view attr_value) const {
    switch (m_operator) {
        case AttributeOperator::Exists:
            return true;

        case AttributeOperator::Equals:
            return attr_value == m_value;

        case AttributeOperator::Contains:
            return attr_value.find(m_value) != std::string_view::npos;

        case AttributeOperator::StartsWith:
            return attr_value.starts_with(m_value);

        case AttributeOperator::EndsWith:
            return attr_value.ends_with(m_value);

        case AttributeOperator::WordMatch: {
            size_t pos = 0;
            while (pos < attr_value.size()) {
                while (pos < attr_value.size() && is_whitespace(attr_value[pos])) {
                    ++pos;
                }
                if (pos >= attr_value.size()) {
                    break;
                }

                size_t end = pos;
                while (end < attr_value.size() && !is_whitespace(attr_value[end])) {
                    ++end;
                }

                if (attr_value.substr(pos, end - pos) == m_value) {
                    return true;
                }

                pos = end;
            }
            return false;
        }

        case AttributeOperator::LangMatch:
            // 语言匹配：属性值等于目标值，或以"目标值-"开头
            return attr_value == m_value || (attr_value.length() > m_value.length() && attr_value.starts_with(m_value) && attr_value[m_value.length()] == '-');
    }
    return false;
}

// ==================== DescendantSelector Implementation ====================

bool DescendantSelector::matches(const Element& element) const {
    // 检查右侧选择器是否匹配当前元素
    if (!m_right || !m_right->matches(element)) {
        return false;
    }

    // 检查是否有祖先元素匹配左侧选择器
    if (!m_left) {
        return true;
    }

    auto parent = element.parent();
    while (parent) {                 // 遍历所有父节点，直到根节点
        if (parent->is_element()) {  // 只有当父节点是元素时才尝试匹配左侧选择器
            auto parent_element = parent->as_element();
            if (parent_element && m_left->matches(*parent_element)) {
                return true;
            }
        }
        parent = parent->parent();
    }
    return false;
}

std::string DescendantSelector::to_string() const {
    std::string result;
    if (m_left) {
        result += m_left->to_string();
    }
    result += " ";
    if (m_right) {
        result += m_right->to_string();
    }
    return result;
}

// ==================== ChildSelector Implementation ====================

bool ChildSelector::matches(const Element& element) const {
    // 检查右侧选择器是否匹配当前元素
    if (!m_right || !m_right->matches(element)) {
        return false;
    }

    // 检查直接父元素是否匹配左侧选择器
    if (!m_left) {
        return true;
    }

    const auto parent = element.parent();
    if (!parent || !parent->is_element()) {
        return false;
    }

    const auto parent_element = parent->as_element();
    if (!parent_element) {
        return false;
    }
    return m_left->matches(*parent_element);
}

std::string ChildSelector::to_string() const {
    std::string result;
    if (m_left) {
        result += m_left->to_string();
    }
    result += " > ";
    if (m_right) {
        result += m_right->to_string();
    }
    return result;
}

// ==================== AdjacentSiblingSelector Implementation ====================

bool AdjacentSiblingSelector::matches(const Element& element) const {
    // 相邻兄弟选择器 A + B：B元素紧跟在A元素之后，且它们有相同的父元素

    // 首先检查右侧选择器是否匹配当前元素
    if (!m_right || !m_right->matches(element)) {
        return false;
    }

    // 如果没有左侧选择器，则总是匹配
    if (!m_left) {
        return true;
    }

    for (auto previous = element.previous_sibling(); previous; previous = previous->previous_sibling()) {
        if (previous->is_element()) {
            return m_left->matches(*previous->as_element());
        }
    }

    return false;
}

std::string AdjacentSiblingSelector::to_string() const {
    std::string result;
    if (m_left) {
        result += m_left->to_string();
    }
    result += " + ";
    if (m_right) {
        result += m_right->to_string();
    }
    return result;
}

// ==================== GeneralSiblingSelector Implementation ====================

bool GeneralSiblingSelector::matches(const Element& element) const {
    // 通用兄弟选择器 A ~ B：B元素在A元素之后的任意位置，且它们有相同的父元素

    // 首先检查右侧选择器是否匹配当前元素
    if (!m_right || !m_right->matches(element)) {
        return false;
    }

    // 如果没有左侧选择器，则总是匹配
    if (!m_left) {
        return true;
    }

    for (auto previous = element.previous_sibling(); previous; previous = previous->previous_sibling()) {
        if (previous->is_element()) {
            if (const auto* sibling_element = previous->as_element(); sibling_element && m_left->matches(*sibling_element)) {
                return true;
            }
        }
    }

    return false;
}

std::string GeneralSiblingSelector::to_string() const {
    std::string result;
    if (m_left) {
        result += m_left->to_string();
    }
    result += " ~ ";
    if (m_right) {
        result += m_right->to_string();
    }
    return result;
}

// ==================== CompoundSelector Implementation ====================

void CompoundSelector::add_selector(std::unique_ptr<CSSSelector> selector) {
    if (selector) {
        m_selectors.push_back(std::move(selector));
    }
}

bool CompoundSelector::matches(const Element& element) const {
    // 复合选择器要求所有子选择器都匹配
    for (const auto& selector : m_selectors) {
        if (!selector->matches(element)) {
            return false;
        }
    }
    return !m_selectors.empty();
}

std::string CompoundSelector::to_string() const {
    std::string result;
    for (const auto& selector : m_selectors) {
        result += selector->to_string();
    }
    return result;
}

// ==================== SelectorList Implementation ====================

void SelectorList::add_selector(std::unique_ptr<CSSSelector> selector) {
    if (selector) {
        m_selectors.push_back(std::move(selector));
    }
}

bool SelectorList::matches(const Element& element) const {
    return std::ranges::any_of(m_selectors, [&element](const auto& selector) { return selector->matches(element); });
}

std::string SelectorList::to_string() const {
    std::string result;
    for (size_t i = 0; i < m_selectors.size(); ++i) {
        if (i > 0) {
            result += ", ";
        }
        result += m_selectors[i]->to_string();
    }
    return result;
}

}  // namespace hps
