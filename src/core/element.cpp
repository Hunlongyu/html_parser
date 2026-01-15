#include "hps/core/element.hpp"

#include "hps/core/text_node.hpp"
#include "hps/parsing/html_parser.hpp"
#include "hps/query/element_query.hpp"
#include "hps/query/query.hpp"
#include "hps/utils/string_utils.hpp"

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

std::string Element::own_text() const {
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
    return std::ranges::any_of(m_attributes, [name](const Attribute& attr) { return equals_ignore_case(attr.name(), name); });
}

const std::string& Element::get_attribute(const std::string_view name) const noexcept {
    static const std::string empty_string;
    const auto               it = std::ranges::find_if(m_attributes, [name](const Attribute& attr) { return equals_ignore_case(attr.name(), name); });
    return it != m_attributes.end() ? it->value() : empty_string;
}

const std::vector<Attribute>& Element::attributes() const noexcept {
    return m_attributes;
}

size_t Element::attribute_count() const noexcept {
    return m_attributes.size();
}

const std::string& Element::id() const noexcept {
    return get_attribute("id");
}

const std::string& Element::class_name() const noexcept {
    return get_attribute("class");
}

std::unordered_set<std::string> Element::class_names() const noexcept {
    const std::string& cls = get_attribute("class");
    if (!cls.empty()) {
        return split_class_names(cls);
    }
    return {};
}

bool Element::has_class(const std::string_view class_name) const noexcept {
    if (class_name.empty()) {
        return false;
    }
    const std::string& attr_val = get_attribute("class");
    if (attr_val.empty()) {
        return false;
    }
    const std::string_view val = attr_val;
    size_t                 pos = 0;
    const size_t           len = val.length();

    while (pos < len) {
        while (pos < len && is_whitespace(val[pos])) {
            ++pos;
        }
        if (pos >= len) {
            break;
        }
        size_t end = pos;
        while (end < len && !is_whitespace(val[end])) {
            ++end;
        }
        if (val.substr(pos, end - pos) == class_name) {
            return true;
        }
        pos = end;
    }
    return false;
}

const Element* Element::querySelector(const std::string_view selector) const {
    return Query::css(*this, selector).first_element();
}

std::vector<const Element*> Element::querySelectorAll(const std::string_view selector) const {
    return Query::css(*this, selector).elements();
}

const Element* Element::get_element_by_id(const std::string_view id) const {
    std::function<const Element*(const Node*)> find_by_id = [&](const Node* node) -> const Element* {
        if (!node || !node->is_element()) {
            return nullptr;
        }
        const auto element = node->as_element();
        if (element->id() == id) {
            return element;
        }
        for (const auto& child : element->children()) {
            if (const auto found = find_by_id(child)) {
                return found;
            }
        }
        return nullptr;
    };
    for (const auto& child : children()) {
        if (const auto found = find_by_id(child)) {
            return found;
        }
    }
    return nullptr;
}

std::vector<const Element*> Element::get_elements_by_tag_name(const std::string_view tag_name) const {
    std::vector<const Element*>         result;
    std::function<void(const Element*)> collect = [&](const Element* el) {
        for (const auto& child : el->children()) {
            if (child->is_element()) {
                auto element_child = child->as_element();
                if (equals_ignore_case(element_child->tag_name(), tag_name)) {
                    result.push_back(element_child);
                }
                collect(element_child);
            }
        }
    };
    collect(this);
    return result;
}

std::vector<const Element*> Element::get_elements_by_class_name(const std::string_view class_name) const {
    std::vector<const Element*> result;

    std::function<void(const Element*)> collect = [&](const Element* el) {
        for (const auto& child : el->children()) {
            if (child->is_element()) {
                auto element_child = child->as_element();
                if (element_child->has_class(class_name)) {
                    result.push_back(element_child);
                }
                collect(element_child);
            }
        }
    };
    collect(this);
    return result;
}

ElementQuery Element::css(const std::string_view selector) const {
    return Query::css(*this, selector);
}

void Element::add_child(std::unique_ptr<Node> child) {
    if (!child) {
        return;
    }
    append_child(std::move(child));
}

void Element::add_attribute(std::string_view name, std::string_view value) {
    const auto it = std::ranges::find_if(m_attributes, [name](const Attribute& attr) { return equals_ignore_case(attr.name(), name); });
    if (it != m_attributes.end()) {
        it->set_value(value);
    } else {
        m_attributes.emplace_back(name, value);
    }
}

}  // namespace hps
