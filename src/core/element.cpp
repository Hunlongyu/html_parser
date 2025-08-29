#include "hps/core/element.hpp"

namespace hps {
Element::Element(std::string_view tag_name) : Node(NodeType::Element), m_tag_name(tag_name) {}

NodeType Element::node_type() const {
    return NodeType::Element;
}

std::string_view Element::node_name() const {
    return m_tag_name;
}

std::string_view Element::node_value() const {
    return {};
}

std::string_view Element::text_content() const {
    return {};
}

std::string_view Element::tag_name() const noexcept {
    return m_tag_name;
}

bool Element::has_attribute(std::string_view name) const noexcept {
    return false;
}

std::string_view Element::get_attribute(std::string_view name) const noexcept {
    return {};
}

const std::vector<Attribute>& Element::attributes() const noexcept {
    return m_attributes;
}

size_t Element::attribute_count() const noexcept {
    return m_attributes.size();
}

std::string_view Element::id() const noexcept {
    return get_attribute("id");
}

const std::unordered_set<std::string_view>& Element::class_names() const noexcept {
    return {};
}

std::string_view Element::class_name() const noexcept {
    return get_attribute("class");
}

bool Element::has_class(std::string_view class_name) const noexcept {
    return false;
}
std::vector<const Element*> Element::get_elements_by_tag_name(std::string_view tag_name) const {
    return {};
}

std::vector<const Element*> Element::get_elements_by_class_name(std::string_view class_name) const {
    return {};
}

const Element* Element::get_element_by_id(std::string_view id) const {
    return nullptr;
}

void add_attribute(std::string_view name, std::string_view value);

}  // namespace hps