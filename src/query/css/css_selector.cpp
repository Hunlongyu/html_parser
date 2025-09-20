#include "hps/query/css/css_selector.hpp"

#include "hps/core/element.hpp"

#include <algorithm>
#include <sstream>

namespace hps {

// ==================== TypeSelector Implementation ====================

bool TypeSelector::matches(const Element& element) const {
    return element.tag_name() == m_tag_name;
}

bool TypeSelector::can_quick_reject(const Element& element) const {
    return element.tag_name() != m_tag_name;
}

// ==================== ClassSelector Implementation ====================

bool ClassSelector::matches(const Element& element) const {
    if (!element.has_attribute("class")) {
        return false;
    }

    const std::string class_attr = element.get_attribute("class");
    if (class_attr.empty()) {
        return false;
    }

    // 分割class属性值，检查是否包含目标类名
    std::istringstream iss(class_attr);
    std::string        class_name;
    while (iss >> class_name) {
        if (class_name == m_class_name) {
            return true;
        }
    }
    return false;
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

    const std::string attr_value = element.get_attribute(m_attr_name);
    return matches_attribute_value(attr_value);
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
            const std::string  attr_str(attr_value);
            std::istringstream iss(attr_str);
            std::string        word;
            while (iss >> word) {
                if (word == m_value) {
                    return true;
                }
            }
            return false;
        }

        case AttributeOperator::LangMatch:
            // 语言匹配：属性值等于目标值，或以"目标值-"开头
            return attr_value == m_value || (attr_value.length() > m_value.length() && attr_value.starts_with(m_value) && attr_value[m_value.length()] == '-');
    }
    return false;
}

std::string AttributeSelector::to_string() const {
    std::string result = "[" + m_attr_name;

    switch (m_operator) {
        case AttributeOperator::Exists:
            break;
        case AttributeOperator::Equals:
            result += "=\"" + m_value + "\"";
            break;
        case AttributeOperator::Contains:
            result += "*=\"" + m_value + "\"";
            break;
        case AttributeOperator::StartsWith:
            result += "^=\"" + m_value + "\"";
            break;
        case AttributeOperator::EndsWith:
            result += "$=\"" + m_value + "\"";
            break;
        case AttributeOperator::WordMatch:
            result += "~=\"" + m_value + "\"";
            break;
        case AttributeOperator::LangMatch:
            result += "|=\"" + m_value + "\"";
            break;
    }

    result += "]";
    return result;
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

    // 获取父元素
    const auto parent = element.parent();
    if (!parent) {
        return false;  // 没有父元素，无法有兄弟元素
    }

    // 获取所有子元素
    const auto& siblings = parent->children();

    // 找到当前元素在兄弟列表中的位置
    auto current_it = std::ranges::find_if(siblings, [&element](const std::shared_ptr<const Node>& node) { return node.get() == &element; });

    if (current_it == siblings.begin()) {
        return false;  // 当前元素是第一个，没有前一个兄弟
    }

    // 向前查找紧邻的元素节点（跳过文本节点和注释节点）
    auto prev_it = current_it;
    while (prev_it != siblings.begin()) {
        --prev_it;
        if ((*prev_it)->is_element()) {
            // 找到了前一个元素节点
            if (const auto prev_element = (*prev_it)->as_element()) {
                // 检查前一个元素是否匹配左侧选择器
                return m_left->matches(*prev_element);
            }
        }
        // 如果遇到非元素节点，继续向前查找
        // 注意：严格的相邻兄弟选择器要求紧邻，不应跳过文本节点
        // 但在实际实现中，通常会忽略空白文本节点
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

    // 获取父元素
    const auto parent = element.parent();
    if (!parent) {
        return false;  // 没有父元素，无法有兄弟元素
    }

    // 获取所有子元素
    const auto& siblings = parent->children();

    // 找到当前元素在兄弟列表中的位置
    auto current_it = std::ranges::find_if(siblings, [&element](const std::shared_ptr<const Node>& node) { return node.get() == &element; });

    if (current_it == siblings.begin()) {
        return false;  // 当前元素是第一个，没有前面的兄弟
    }

    // 检查当前元素之前的所有兄弟元素
    for (auto it = siblings.begin(); it != current_it; ++it) {
        if ((*it)->is_element()) {
            auto sibling_element = (*it)->as_element();
            if (sibling_element && m_left->matches(*sibling_element)) {
                return true;  // 找到匹配左侧选择器的前面兄弟元素
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