#include "hps/query/element_query.hpp"

#include "hps/core/element.hpp"

#include <algorithm>
#include <functional>

namespace hps {

// 构造函数
ElementQuery::ElementQuery(std::vector<std::shared_ptr<const Element>>&& elements) : m_elements(std::move(elements)) {}

ElementQuery::ElementQuery(const std::vector<std::shared_ptr<const Element>>& elements) : m_elements(elements) {}

// 基本访问方法
size_t ElementQuery::size() const noexcept {
    return m_elements.size();
}

bool ElementQuery::empty() const noexcept {
    return m_elements.empty();
}

const std::vector<std::shared_ptr<const Element>>& ElementQuery::elements() const {
    return m_elements;
}

std::shared_ptr<const Element> ElementQuery::first_element() const {
    return empty() ? nullptr : m_elements[0];
}

std::shared_ptr<const Element> ElementQuery::last_element() const {
    return empty() ? nullptr : m_elements.back();
}

std::shared_ptr<const Element> ElementQuery::operator[](size_t index) const {
    return at(index);
}

std::shared_ptr<const Element> ElementQuery::at(size_t index) const {
    if (index >= m_elements.size()) {
        return nullptr;
    }
    return m_elements[index];
}

// 迭代器支持
ElementQuery::iterator ElementQuery::begin() {
    return m_elements.begin();
}

ElementQuery::iterator ElementQuery::end() {
    return m_elements.end();
}

ElementQuery::const_iterator ElementQuery::begin() const {
    return m_elements.begin();
}

ElementQuery::const_iterator ElementQuery::end() const {
    return m_elements.end();
}

ElementQuery::const_iterator ElementQuery::cbegin() const {
    return m_elements.cbegin();
}

ElementQuery::const_iterator ElementQuery::cend() const {
    return m_elements.cend();
}

// 条件过滤方法
ElementQuery ElementQuery::has_attribute(std::string_view name) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && element->has_attribute(name)) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::has_attribute(std::string_view name, std::string_view value) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && element->has_attribute(name) && element->get_attribute(name) == value) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::has_class(std::string_view class_name) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && element->has_class(class_name)) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::has_tag(std::string_view tag_name) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && element->tag_name() == tag_name) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::has_text(std::string_view text) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && element->text_content() == text) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::containing_text(std::string_view text) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && element->text_content().find(text) != std::string::npos) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::matching_text(std::function<bool(std::string_view)> predicate) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && predicate(element->text_content())) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

// 索引和范围操作
ElementQuery ElementQuery::slice(size_t start, size_t end) const {
    if (start >= m_elements.size()) {
        return ElementQuery();
    }

    end = std::min(end, m_elements.size());
    if (start >= end) {
        return ElementQuery();
    }

    std::vector<std::shared_ptr<const Element>> sliced(m_elements.begin() + start, m_elements.begin() + end);
    return ElementQuery(std::move(sliced));
}

ElementQuery ElementQuery::first(size_t n) const {
    return slice(0, n);
}

ElementQuery ElementQuery::last(size_t n) const {
    if (n >= m_elements.size()) {
        return *this;
    }
    return slice(m_elements.size() - n, m_elements.size());
}

ElementQuery ElementQuery::skip(size_t n) const {
    if (n >= m_elements.size()) {
        return ElementQuery();
    }
    return slice(n, m_elements.size());
}

ElementQuery ElementQuery::limit(size_t n) const {
    return first(n);
}

// 导航方法
ElementQuery ElementQuery::children() const {
    std::vector<std::shared_ptr<const Element>> all_children;
    for (const auto& element : m_elements) {
        if (element) {
            auto children = element->children();
            for (const auto& child : children) {
                if (auto elem = std::dynamic_pointer_cast<const Element>(child)) {
                    all_children.push_back(elem);
                }
            }
        }
    }
    return ElementQuery(std::move(all_children));
}

ElementQuery ElementQuery::children(std::string_view selector) const {
    // 实现CSS选择器过滤的children方法
    return children().filter([selector](const Element& elem) {
        // 这里需要实现CSS选择器匹配逻辑
        return true;  // 临时实现
    });
}

ElementQuery ElementQuery::parent() const {
    std::vector<std::shared_ptr<const Element>> parents;
    for (const auto& element : m_elements) {
        if (element && element->parent()) {
            if (auto parent_elem = std::dynamic_pointer_cast<const Element>(element->parent())) {
                parents.push_back(parent_elem);
            }
        }
    }
    return ElementQuery(std::move(parents));
}

ElementQuery ElementQuery::parents() const {
    std::vector<std::shared_ptr<const Element>> all_parents;
    for (const auto& element : m_elements) {
        if (element) {
            auto current = element->parent();
            while (current) {
                if (auto parent_elem = std::dynamic_pointer_cast<const Element>(current)) {
                    all_parents.push_back(parent_elem);
                }
                current = current->parent();
            }
        }
    }
    return ElementQuery(std::move(all_parents));
}

ElementQuery ElementQuery::closest(std::string_view selector) const {
    std::vector<std::shared_ptr<const Element>> closest_elements;
    for (const auto& element : m_elements) {
        if (element) {
            auto current = element;
            while (current) {
                // 这里需要实现CSS选择器匹配逻辑
                // 临时实现：假设匹配成功
                closest_elements.push_back(current);
                break;

                // 向上遍历
                if (auto parent = std::dynamic_pointer_cast<const Element>(current->parent())) {
                    current = parent;
                } else {
                    break;
                }
            }
        }
    }
    return ElementQuery(std::move(closest_elements));
}

ElementQuery ElementQuery::next_sibling() const {
    std::vector<std::shared_ptr<const Element>> siblings;
    for (const auto& element : m_elements) {
        if (element && element->parent()) {
            auto parent_children = element->parent()->children();
            auto it              = std::find(parent_children.begin(), parent_children.end(), element);
            if (it != parent_children.end() && ++it != parent_children.end()) {
                if (auto sibling_elem = std::dynamic_pointer_cast<const Element>(*it)) {
                    siblings.push_back(sibling_elem);
                }
            }
        }
    }
    return ElementQuery(std::move(siblings));
}

ElementQuery ElementQuery::next_siblings() const {
    std::vector<std::shared_ptr<const Element>> siblings;
    for (const auto& element : m_elements) {
        if (element && element->parent()) {
            auto parent_children = element->parent()->children();
            auto it              = std::find(parent_children.begin(), parent_children.end(), element);
            if (it != parent_children.end()) {
                for (++it; it != parent_children.end(); ++it) {
                    if (auto sibling_elem = std::dynamic_pointer_cast<const Element>(*it)) {
                        siblings.push_back(sibling_elem);
                    }
                }
            }
        }
    }
    return ElementQuery(std::move(siblings));
}

ElementQuery ElementQuery::prev_sibling() const {
    std::vector<std::shared_ptr<const Element>> siblings;
    for (const auto& element : m_elements) {
        if (element && element->parent()) {
            auto parent_children = element->parent()->children();
            auto it              = std::find(parent_children.begin(), parent_children.end(), element);
            if (it != parent_children.begin()) {
                --it;
                if (auto sibling_elem = std::dynamic_pointer_cast<const Element>(*it)) {
                    siblings.push_back(sibling_elem);
                }
            }
        }
    }
    return ElementQuery(std::move(siblings));
}

ElementQuery ElementQuery::prev_siblings() const {
    std::vector<std::shared_ptr<const Element>> siblings;
    for (const auto& element : m_elements) {
        if (element && element->parent()) {
            auto parent_children = element->parent()->children();
            auto it              = std::find(parent_children.begin(), parent_children.end(), element);
            for (auto prev_it = parent_children.begin(); prev_it != it; ++prev_it) {
                if (auto sibling_elem = std::dynamic_pointer_cast<const Element>(*prev_it)) {
                    siblings.push_back(sibling_elem);
                }
            }
        }
    }
    return ElementQuery(std::move(siblings));
}

ElementQuery ElementQuery::siblings() const {
    std::vector<std::shared_ptr<const Element>> siblings;
    for (const auto& element : m_elements) {
        if (element && element->parent()) {
            auto parent_children = element->parent()->children();
            for (const auto& child : parent_children) {
                if (child != element) {
                    if (auto sibling_elem = std::dynamic_pointer_cast<const Element>(child)) {
                        siblings.push_back(sibling_elem);
                    }
                }
            }
        }
    }
    return ElementQuery(std::move(siblings));
}

ElementQuery ElementQuery::css(std::string_view selector) const {
    return {};
}

ElementQuery ElementQuery::xpath(std::string_view expression) const {
    return {};
}

// 高级查询方法
ElementQuery ElementQuery::filter(std::function<bool(const Element&)> predicate) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && predicate(*element)) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::not_(std::string_view selector) const {
    // 实现CSS选择器的not过滤
    return filter([selector](const Element& elem) {
        // 这里需要实现CSS选择器匹配逻辑
        return true;  // 临时实现
    });
}

ElementQuery ElementQuery::even() const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (size_t i = 0; i < m_elements.size(); i += 2) {
        filtered.push_back(m_elements[i]);
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::odd() const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (size_t i = 1; i < m_elements.size(); i += 2) {
        filtered.push_back(m_elements[i]);
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::eq(size_t index) const {
    if (index < m_elements.size()) {
        std::vector<std::shared_ptr<const Element>> single = {m_elements[index]};
        return ElementQuery(std::move(single));
    }
    return ElementQuery();
}

ElementQuery ElementQuery::gt(size_t index) const {
    if (index + 1 < m_elements.size()) {
        return slice(index + 1, m_elements.size());
    }
    return ElementQuery();
}

ElementQuery ElementQuery::lt(size_t index) const {
    return slice(0, index);
}

// 聚合方法
std::vector<std::string> ElementQuery::extract_attributes(std::string_view attr_name) const {
    std::vector<std::string> attributes;
    for (const auto& element : m_elements) {
        if (element && element->has_attribute(attr_name)) {
            attributes.push_back(element->get_attribute(attr_name));
        }
    }
    return attributes;
}

std::vector<std::string> ElementQuery::extract_texts() const {
    std::vector<std::string> texts;
    for (const auto& element : m_elements) {
        if (element) {
            texts.push_back(element->text_content());
        }
    }
    return texts;
}

std::vector<std::string> ElementQuery::extract_own_texts() const {
    std::vector<std::string> texts;
    for (const auto& element : m_elements) {
        if (element) {
            texts.push_back(element->text_content());
        }
    }
    return texts;
}

// 遍历方法
ElementQuery ElementQuery::each(std::function<void(const Element&)> callback) const {
    for (const auto& element : m_elements) {
        if (element) {
            callback(*element);
        }
    }
    return *this;
}

ElementQuery ElementQuery::each(std::function<void(size_t, const Element&)> callback) const {
    for (size_t i = 0; i < m_elements.size(); ++i) {
        if (m_elements[i]) {
            callback(i, *m_elements[i]);
        }
    }
    return *this;
}

// 布尔检查方法
bool ElementQuery::is(std::string_view selector) const {
    // 实现CSS选择器匹配检查
    for (const auto& element : m_elements) {
        if (element) {
            // 这里需要实现CSS选择器匹配逻辑
            return true;  // 临时实现
        }
    }
    return false;
}

bool ElementQuery::contains(const Element& element) const {
    for (const auto& elem : m_elements) {
        if (elem.get() == &element) {
            return true;
        }
    }
    return false;
}

bool ElementQuery::contains(std::string_view text) const {
    for (const auto& element : m_elements) {
        if (element && element->text_content().find(text) != std::string::npos) {
            return true;
        }
    }
    return false;
}

}  // namespace hps