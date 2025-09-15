#include "hps/core/element.hpp"

#include "hps/core/text_node.hpp"
#include "hps/parsing/html_parser.hpp"
#include "hps/query/element_query.hpp"
#include "hps/utils/string_utils.hpp"

#include <algorithm>
#include <sstream>

namespace hps {
Element::Element(const std::string_view name) : Node(NodeType::Element), m_name(name) {}

NodeType Element::type() {
    return NodeType::Element;
}

std::string Element::text_content() const {
    std::stringstream ss;
    for (const auto& child : children()) {
        ss << child->text_content();
    }
    return ss.str();
}

const std::string& Element::tag_name() const noexcept {
    return m_name;
}

bool Element::has_attribute(const std::string_view name) const noexcept {
    return std::ranges::any_of(m_attributes, [&](const auto& attr) { return attr.name() == name; });
}

std::string Element::get_attribute(const std::string_view name) const noexcept {
    for (const auto& attr : m_attributes) {
        if (attr.name() == name) {
            return attr.value();
        }
    }
    return "";
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
    const std::string class_attr = get_attribute("class");
    if (class_attr.empty()) {
        return {};
    }
    return split_class_names(class_attr);
}

bool Element::has_class(const std::string_view class_name) const noexcept {
    const std::string class_attr = get_attribute("class");
    if (class_attr.empty()) {
        return false;
    }
    const auto class_names = split_class_names(class_attr);
    return class_names.contains(std::string(class_name));
}

std::shared_ptr<const Element> Element::querySelector(std::string_view selector) const {
    // TODO: Implement logic for CSS selector query. This will likely involve a CSS selector engine.
    return nullptr;
}

std::vector<std::shared_ptr<const Element>> Element::querySelectorAll(std::string_view selector) const {
    // TODO: Implement logic for CSS selector query. This will likely involve a CSS selector engine.
    return {};
}

std::shared_ptr<const Element> Element::get_element_by_id(std::string_view id) const {
    // TODO: Implement logic to find an element by ID among its descendants.
    // This will require a recursive search.
    return nullptr;
}

std::vector<std::shared_ptr<const Element>> Element::get_elements_by_tag_name(const std::string_view tag_name) const {
    // TODO: Implement logic to find direct children by tag name.
    std::vector<std::shared_ptr<const Element>> result;
    for (const auto& child : children()) {
        if (child->type() == NodeType::Element) {
            auto element_child = std::static_pointer_cast<const Element>(child);
            if (element_child->tag_name() == tag_name) {
                result.push_back(element_child);
            }
        }
    }
    return result;
}

std::vector<std::shared_ptr<const Element>> Element::get_elements_by_class_name(const std::string_view class_name) const {
    // TODO: Implement logic to find direct children by class name.
    std::vector<std::shared_ptr<const Element>> result;
    for (const auto& child : children()) {
        if (child->type() == NodeType::Element) {
            auto element_child = std::static_pointer_cast<const Element>(child);
            if (element_child->has_class(class_name)) {
                result.push_back(element_child);
            }
        }
    }
    return result;
}

ElementQuery Element::css(std::string_view selector) const {
    // TODO: Implement ElementQuery integration.
    // This will likely involve creating an ElementQuery object with the current element as context.
    return {};
}

ElementQuery Element::xpath(std::string_view expression) const {
    // TODO: Implement ElementQuery integration.
    // This will likely involve creating an ElementQuery object with the current element as context.
    return {};
}

void Element::add_child(const std::shared_ptr<Node>& child) {
    if (!child) {
        return;
    }
    append_child(child);
}

void Element::add_attribute(std::string_view name, std::string_view value) {
    for (auto& attr : m_attributes) {
        if (attr.name() == name) {
            attr.set_value(value);
            return;
        }
    }
    m_attributes.emplace_back(name, value);
}

}  // namespace hps