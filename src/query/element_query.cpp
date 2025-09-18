#include "hps/query/element_query.hpp"

#include "hps/core/element.hpp"
#include "hps/query/css/css_parser.hpp"
#include "hps/query/query.hpp"

#include <algorithm>
#include <span>

namespace hps {
ElementQuery::ElementQuery(const Element& element) {
    if (const auto shared_element = element.shared_from_this()->as_element()) {
        m_elements.push_back(shared_element);
    }
}

ElementQuery::ElementQuery(const std::shared_ptr<const Element>& element) {
    if (element) {
        m_elements.push_back(element);
    }
}

ElementQuery::ElementQuery(std::vector<std::shared_ptr<const Element>>&& elements)
    : m_elements(std::move(elements)) {}

ElementQuery::ElementQuery(const std::vector<std::shared_ptr<const Element>>& elements)
    : m_elements(elements) {}

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

std::shared_ptr<const Element> ElementQuery::operator[](const size_t index) const {
    return at(index);
}

std::shared_ptr<const Element> ElementQuery::at(const size_t index) const {
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
ElementQuery ElementQuery::has_attribute(const std::string_view name) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && element->has_attribute(name)) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::has_attribute(const std::string_view name, const std::string_view value) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && element->has_attribute(name) && element->get_attribute(name) == value) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::has_class(const std::string_view class_name) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && element->has_class(class_name)) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::has_tag(const std::string_view tag_name) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && element->tag_name() == tag_name) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::has_text(const std::string_view text) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && element->text_content() == text) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::containing_text(const std::string_view text) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && element->text_content().find(text) != std::string::npos) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::matching_text(const std::function<bool(std::string_view)>& predicate) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && predicate(element->text_content())) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

// 索引和范围操作
ElementQuery ElementQuery::slice(const size_t start, size_t end) const {
    const size_t size = m_elements.size();
    if (start >= size) {
        return {};
    }
    end = std::min(end, size);
    if (start >= end) {
        return {};
    }

    auto span_view = std::span(m_elements).subspan(start, end - start);
    return ElementQuery({span_view.begin(), span_view.end()});
}

ElementQuery ElementQuery::first(const size_t n) const {
    return slice(0, n);
}

ElementQuery ElementQuery::last(const size_t n) const {
    if (n >= m_elements.size()) {
        return *this;
    }
    return slice(m_elements.size() - n, m_elements.size());
}

ElementQuery ElementQuery::skip(const size_t n) const {
    if (n >= m_elements.size()) {
        return {};
    }
    return slice(n, m_elements.size());
}

ElementQuery ElementQuery::limit(const size_t n) const {
    return first(n);
}

// 导航方法
ElementQuery ElementQuery::children() const {
    std::vector<std::shared_ptr<const Element>> all_children;
    for (const auto& element : m_elements) {
        if (element) {
            auto children = element->children();
            for (const auto& child : children) {
                if (auto elem = child->as_element()) {
                    all_children.push_back(elem);
                }
            }
        }
    }
    return ElementQuery(std::move(all_children));
}

ElementQuery ElementQuery::children(const std::string_view selector) const {
    if (selector.empty()) {
        return children();
    }

    auto selector_list = parse_css_selector(selector);
    if (!selector_list) {
        return {};
    }
    return children().filter([&selector_list](const Element& elem) { return selector_list->matches(elem); });
}

ElementQuery ElementQuery::parent() const {
    std::vector<std::shared_ptr<const Element>> parents;
    for (const auto& element : m_elements) {
        if (element && element->parent()) {
            if (auto parent_elem = element->parent()->as_element()) {
                parents.push_back(parent_elem);
            }
        }
    }
    return ElementQuery(std::move(parents));
}

ElementQuery ElementQuery::parents() const {
    std::vector<std::shared_ptr<const Element>>        all_parents;
    std::unordered_set<std::shared_ptr<const Element>> seen;
    for (const auto& element : m_elements) {
        if (element) {
            auto current = element->parent();
            while (current) {
                if (auto parent_elem = current->as_element()) {
                    if (!seen.contains(parent_elem)) {
                        all_parents.push_back(parent_elem);
                        seen.insert(parent_elem);
                    }
                }
                current = current->parent();
            }
        }
    }
    return ElementQuery(std::move(all_parents));
}

ElementQuery ElementQuery::closest(const std::string_view selector) const {
    if (selector.empty()) {
        return {};
    }
    auto selector_list = parse_css_selector(selector);
    if (!selector_list) {
        return {};
    }

    std::vector<std::shared_ptr<const Element>>        closest_elements;
    std::unordered_set<std::shared_ptr<const Element>> seen;
    for (const auto& element : m_elements) {
        if (element) {
            auto current = element;
            while (current) {
                if (selector_list->matches(*current)) {
                    if (!seen.contains(current)) {
                        closest_elements.push_back(current);
                        seen.insert(current);
                    }
                    break;
                }
                const auto parent = current->parent();
                if (parent && parent->is_element()) {
                    current = parent->as_element();
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
        if (const auto ele = element->next_sibling(); ele && ele->is_element()) {
            siblings.push_back(ele->as_element());
        }
    }
    return ElementQuery(std::move(siblings));
}

ElementQuery ElementQuery::next_siblings() const {
    std::vector<std::shared_ptr<const Element>> siblings;
    for (const auto& element : m_elements) {
        auto current = element->next_sibling();
        while (current) {
            if (current->is_element()) {
                siblings.push_back(current->as_element());
            }
            current = current->next_sibling();
        }
    }
    return ElementQuery(std::move(siblings));
}

ElementQuery ElementQuery::prev_sibling() const {
    std::vector<std::shared_ptr<const Element>> siblings;
    for (const auto& element : m_elements) {
        if (const auto ele = element->previous_sibling(); ele && ele->is_element()) {
            siblings.push_back(ele->as_element());
        }
    }
    return ElementQuery(std::move(siblings));
}

ElementQuery ElementQuery::prev_siblings() const {
    std::vector<std::shared_ptr<const Element>> siblings;
    for (const auto& element : m_elements) {
        auto current = element->previous_sibling();
        while (current) {
            if (current->is_element()) {
                siblings.push_back(current->as_element());
            }
            current = current->previous_sibling();
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

ElementQuery ElementQuery::css(const std::string_view selector) const {
    if (selector.empty()) {
        return {};
    }
    std::vector<std::shared_ptr<const Element>> all_results;
    for (const auto& element : m_elements) {
        if (element) {
            auto        results         = Query::css(*element, selector);
            const auto& result_elements = results.elements();
            all_results.insert(all_results.end(), result_elements.begin(), result_elements.end());
        }
    }
    std::ranges::sort(all_results);
    all_results.erase(std::ranges::unique(all_results).begin(), all_results.end());
    return ElementQuery(std::move(all_results));
}

ElementQuery ElementQuery::xpath(std::string_view expression) const {
    return {};
}

// 高级查询方法
ElementQuery ElementQuery::filter(const std::function<bool(const Element&)>& predicate) const {
    std::vector<std::shared_ptr<const Element>> filtered;
    for (const auto& element : m_elements) {
        if (element && predicate(*element)) {
            filtered.push_back(element);
        }
    }
    return ElementQuery(std::move(filtered));
}

ElementQuery ElementQuery::not_(const std::string_view selector) const {
    if (selector.empty()) {
        return *this;
    }
    const auto selector_list = parse_css_selector(selector);
    if (!selector_list || selector_list->empty()) {
        return *this;
    }
    return filter([&selector_list](const Element& ele) { return !selector_list->matches(ele); });
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

ElementQuery ElementQuery::eq(const size_t index) const {
    if (index < m_elements.size()) {
        std::vector<std::shared_ptr<const Element>> single = {m_elements[index]};
        return ElementQuery(std::move(single));
    }
    return {};
}

ElementQuery ElementQuery::gt(const size_t index) const {
    if (index + 1 < m_elements.size()) {
        return slice(index + 1, m_elements.size());
    }
    return {};
}

ElementQuery ElementQuery::lt(const size_t index) const {
    return slice(0, index);
}

// 聚合方法
std::vector<std::string> ElementQuery::extract_attributes(const std::string_view attr_name) const {
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

ElementQuery ElementQuery::each(const std::function<void(const Element&)>& callback) const {
    for (const auto& element : m_elements) {
        if (element) {
            callback(*element);
        }
    }
    return *this;
}

ElementQuery ElementQuery::each(const std::function<void(size_t, const Element&)>& callback) const {
    for (size_t i = 0; i < m_elements.size(); ++i) {
        if (m_elements[i]) {
            callback(i, *m_elements[i]);
        }
    }
    return *this;
}

bool ElementQuery::is(const std::string_view selector) const {
    if (selector.empty()) {
        return false;
    }
    const auto selector_list = parse_css_selector(selector);
    if (!selector_list || selector_list->empty()) {
        return false;
    }
    for (const auto& element : m_elements) {
        if (element && selector_list->matches(*element)) {
            return true;
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

bool ElementQuery::contains(const std::string_view text) const {
    auto has_text = [&text](const std::shared_ptr<const Element>& element) { return element && element->text_content().find(text) != std::string::npos; };

    return std::ranges::any_of(m_elements, has_text);
}

}  // namespace hps