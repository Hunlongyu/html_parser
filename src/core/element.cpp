#include "hps/core/element.hpp"

#include "hps/query/element_query.hpp"

#include <algorithm>

namespace hps {
Element::Element(std::string_view name) : Node(NodeType::Element), m_name(name) {
    m_attributes.reserve(8);
}

NodeType Element::node_type() const {
    return NodeType::Element;
}

std::string Element::node_name() const {
    return m_name;
}

std::string Element::node_value() const {
    return {};
}

std::string Element::text_content() const {
    return {};
}

bool Element::has_attribute(std::string_view name) const noexcept {
    return std::ranges::any_of(m_attributes, [name](const Attribute& attr) { return attr.name() == name; });
}

std::string Element::get_attribute(std::string_view name) const noexcept {
    auto it = std::ranges::find_if(m_attributes, [name](const Attribute& attr) { return attr.name() == name; });

    if (it != m_attributes.end()) {
        return it->value();
    }

    return {};
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

std::string Element::tag_name() const noexcept {
    return m_name;
}

const std::unordered_set<std::string_view>& Element::class_names() const noexcept {
    return {};
}

std::string Element::class_name() const noexcept {
    return get_attribute("class");
}

bool Element::has_class(std::string_view class_name) const noexcept {
    return false;
}

const Element* Element::querySelector(std::string_view selector) const {
    return nullptr;
}

std::vector<const Element*> Element::querySelectorAll(std::string_view selector) const {
    return {};
}

const Element* Element::get_element_by_id(std::string_view id) const {
    return nullptr;
}
std::vector<const Element*> Element::get_elements_by_tag_name(std::string_view tag_name) const {
    return {};
}

std::vector<const Element*> Element::get_elements_by_class_name(std::string_view class_name) const {
    return {};
}

ElementQuery Element::css(std::string_view selector) const {
    return {};
}

ElementQuery Element::xpath(std::string_view expression) const {
    return {};
}

void Element::add_attribute(std::string_view name, std::string_view value) {
    auto it = std::ranges::find_if(m_attributes, [name](const Attribute& attr) { return attr.name() == name; });

    if (it != m_attributes.end()) {
        *it = Attribute(name, value);
    } else {
        m_attributes.emplace_back(name, value);
    }
}

void Element::add_child(std::unique_ptr<Node> child) {
    Node::add_child(std::move(child));
}
}  // namespace hps