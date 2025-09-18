#include "hps/core/element.hpp"

#include "hps/core/text_node.hpp"
#include "hps/parsing/html_parser.hpp"
#include "hps/query/element_query.hpp"
#include "hps/query/query.hpp"
#include "hps/utils/string_utils.hpp"

#include <algorithm>
#include <numeric>
#include <ranges>
#include <sstream>

namespace hps {
Element::Element(const std::string_view name)
    : Node(NodeType::Element),
      m_name(name) {}

NodeType Element::type() const noexcept {
    return NodeType::Element;
}

std::string Element::text_content() const {
    std::stringstream ss;
    for (const auto& child : children()) {
        ss << child->text_content();
    }
    return ss.str();
}

std::string Element::own_text_content() const {
    std::stringstream ss;
    for (const auto& child : children()) {
        if (child->is_text()) {
            ss << child->as_text()->value();
        }
    }
    return ss.str();
}

const std::string& Element::tag_name() const noexcept {
    return m_name;
}

bool Element::has_attribute(const std::string_view name) const noexcept {
    return std::ranges::any_of(m_attributes, [name](const Attribute& attr) { return attr.name() == name; });
}

std::string Element::get_attribute(const std::string_view name) const noexcept {
    const auto it = std::ranges::find_if(m_attributes, [name](const Attribute& attr) { return attr.name() == name; });
    return it != m_attributes.end() ? it->value() : "";
}

const std::vector<Attribute>& Element::attributes() const noexcept {
    return m_attributes;
}

size_t Element::attribute_count() const noexcept {
    return m_attributes.size();
}

std::string Element::id() const noexcept {
    return get_attribute("id");
}

std::string Element::class_name() const noexcept {
    return get_attribute("class");
}

std::unordered_set<std::string> Element::class_names() const noexcept {
    for (const auto& attr : m_attributes) {
        if (attr.name() == "class") {
            return split_class_names(attr.value());
        }
    }
    return {};
}

bool Element::has_class(const std::string_view class_name) const noexcept {
    for (const auto& attr : m_attributes) {
        if (attr.name() == "class") {
            return split_class_names(attr.value()).contains(std::string(class_name));
        }
    }
    return false;
}

std::shared_ptr<const Element> Element::querySelector(const std::string_view selector) const {
    const auto elements = Query::css(*this, selector);
    return elements.first_element();
}

std::vector<std::shared_ptr<const Element>> Element::querySelectorAll(const std::string_view selector) const {
    return Query::css(*this, selector).elements();
}

std::shared_ptr<const Element> Element::get_element_by_id(std::string_view id) const {
    std::function<std::shared_ptr<const Element>(const std::shared_ptr<const Node>&)> find_by_id = [&](const std::shared_ptr<const Node>& node) -> std::shared_ptr<const Element> {
        if (!node || node->type() != NodeType::Element) {
            return nullptr;
        }
        auto element = node->as_element();
        if (element->id() == id) {
            return element;
        }
        for (const auto& child : element->children()) {
            if (auto found = find_by_id(child)) {
                return found;
            }
        }
        return nullptr;
    };
    for (const auto& child : children()) {
        if (auto found = find_by_id(child)) {
            return found;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<const Element>> Element::get_elements_by_tag_name(const std::string_view tag_name) const {
    std::vector<std::shared_ptr<const Element>> result;
    result.reserve(children().size());
    for (const auto& child : children()) {
        if (child->type() == NodeType::Element) {
            auto element_child = child->as_element();
            if (element_child->tag_name() == tag_name) {
                result.push_back(element_child);
            }
        }
    }
    return result;
}

std::vector<std::shared_ptr<const Element>> Element::get_elements_by_class_name(const std::string_view class_name) const {
    std::vector<std::shared_ptr<const Element>> result;
    result.reserve(children().size());
    for (const auto& child : children()) {
        if (child->type() == NodeType::Element) {
            auto element_child = child->as_element();
            if (element_child->has_class(class_name)) {
                result.push_back(element_child);
            }
        }
    }
    return result;
}

ElementQuery Element::css(const std::string_view selector) const {
    return Query::css(*this, selector);
}

ElementQuery Element::xpath(std::string_view expression) const {
    // TODO: Implement ElementQuery integration.
    return {};
}

void Element::add_child(const std::shared_ptr<Node>& child) {
    if (!child) {
        return;
    }
    append_child(child);
}

void Element::add_attribute(std::string_view name, std::string_view value) {
    const auto it = std::ranges::find_if(m_attributes, [name](const Attribute& attr) { return attr.name() == name; });
    if (it != m_attributes.end()) {
        it->set_value(value);
    } else {
        m_attributes.emplace_back(name, value);
    }
}

}  // namespace hps