#include "hps/core/element.hpp"

#include "hps/core/text_node.hpp"
#include "hps/query/element_query.hpp"
#include "hps/query/query.hpp"
#include "hps/utils/string_utils.hpp"

#include "serializer.hpp"

#include <algorithm>
#include <sstream>

namespace hps {
namespace {

// 在 element 的后代中深度优先查找首个 id 匹配的元素（不含 element 自身）。
const Element* find_descendant_by_id(const Element& element, const std::string_view id) {
    for (const Node* child = element.first_child(); child != nullptr; child = child->next_sibling()) {
        if (const Element* child_element = child->as_element()) {
            if (child_element->id() == id) {
                return child_element;
            }
            if (const Element* found = find_descendant_by_id(*child_element, id)) {
                return found;
            }
        }
    }
    return nullptr;
}

// 在 element 的后代中收集所有标签名匹配的元素（不含 element 自身）。
void collect_descendants_by_tag(const Element& element, const std::string_view tag_name, std::vector<const Element*>& out) {
    for (const Node* child = element.first_child(); child != nullptr; child = child->next_sibling()) {
        if (const Element* child_element = child->as_element()) {
            if (equals_ignore_case(child_element->tag_name(), tag_name)) {
                out.push_back(child_element);
            }
            collect_descendants_by_tag(*child_element, tag_name, out);
        }
    }
}

// 在 element 的后代中收集所有含指定 class 的元素（不含 element 自身）。
void collect_descendants_by_class(const Element& element, const std::string_view class_name, std::vector<const Element*>& out) {
    for (const Node* child = element.first_child(); child != nullptr; child = child->next_sibling()) {
        if (const Element* child_element = child->as_element()) {
            if (child_element->has_class(class_name)) {
                out.push_back(child_element);
            }
            collect_descendants_by_class(*child_element, class_name, out);
        }
    }
}

}  // namespace

Element::Element(const std::string_view name, const NamespaceKind namespace_kind)
    : Node(NodeType::Element),
      m_name(name),
      m_namespace_kind(namespace_kind) {}

NodeType Element::type() const noexcept {
    return NodeType::Element;
}

std::string Element::text_content() const {
    std::stringstream ss;
    for (auto child = first_child(); child; child = child->next_sibling()) {
        ss << child->text_content();
    }
    return ss.str();
}

std::string Element::own_text() const {
    std::stringstream ss;
    for (auto child = first_child(); child; child = child->next_sibling()) {
        if (child->is_text()) {
            ss << child->as_text()->value();
        }
    }
    return ss.str();
}

std::string Element::inner_html() const {
    std::string out;
    detail::serialize_inner(*this, out);
    return out;
}

std::string Element::outer_html() const {
    std::string out;
    detail::serialize_node(*this, out);
    return out;
}

const std::string& Element::tag_name() const noexcept {
    return m_name;
}

NamespaceKind Element::namespace_kind() const noexcept {
    return m_namespace_kind;
}

std::string_view Element::namespace_uri() const noexcept {
    switch (m_namespace_kind) {
        case NamespaceKind::Html:
            return "http://www.w3.org/1999/xhtml";
        case NamespaceKind::Svg:
            return "http://www.w3.org/2000/svg";
        case NamespaceKind::MathML:
            return "http://www.w3.org/1998/Math/MathML";
    }
    return "http://www.w3.org/1999/xhtml";
}

bool Element::has_attribute(const std::string_view name) const noexcept {
    return std::ranges::any_of(m_attributes, [name](const Attribute& attr) { return equals_ignore_case(attr.name(), name); });
}

const std::string& Element::get_attribute(const std::string_view name) const noexcept {
    static const std::string empty_string;
    const auto               it = std::ranges::find_if(m_attributes, [name](const Attribute& attr) { return equals_ignore_case(attr.name(), name); });
    return it != m_attributes.end() ? it->value() : empty_string;
}

std::optional<std::string_view> Element::attr(const std::string_view name) const noexcept {
    const auto it = std::ranges::find_if(m_attributes, [name](const Attribute& attr) { return equals_ignore_case(attr.name(), name); });
    if (it == m_attributes.end()) {
        return std::nullopt;
    }
    return std::string_view(it->value());
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

const Element* Element::query_selector(const std::string_view selector) const {
    return Query::css_first(*this, selector);
}

std::vector<const Element*> Element::query_selector_all(const std::string_view selector) const {
    return Query::css(*this, selector).elements();
}

const Element* Element::get_element_by_id(const std::string_view id) const {
    return find_descendant_by_id(*this, id);
}

std::vector<const Element*> Element::get_elements_by_tag_name(const std::string_view tag_name) const {
    std::vector<const Element*> result;
    collect_descendants_by_tag(*this, tag_name, result);
    return result;
}

std::vector<const Element*> Element::get_elements_by_class_name(const std::string_view class_name) const {
    std::vector<const Element*> result;
    collect_descendants_by_class(*this, class_name, result);
    return result;
}

ElementQuery Element::css(const std::string_view selector) const {
    return Query::css(*this, selector);
}

Node* Element::add_child(std::unique_ptr<Node> child) {
    if (!child) {
        return nullptr;
    }
    Node* inserted = append_child(std::move(child));
    invalidate_document_query_cache();
    return inserted;
}

Node* Element::insert_child_before(std::unique_ptr<Node> child, const Node* before) {
    if (!child) {
        return nullptr;
    }
    Node* inserted = Node::insert_child_before(std::move(child), before);
    invalidate_document_query_cache();
    return inserted;
}

std::vector<std::unique_ptr<Node>> Element::take_children() {
    auto children = Node::take_children();
    invalidate_document_query_cache();
    return children;
}

void Element::add_attribute(std::string_view name, std::string_view value, const bool has_value) {
    const auto it = std::ranges::find_if(m_attributes, [name](const Attribute& attr) { return equals_ignore_case(attr.name(), name); });
    if (it != m_attributes.end()) {
        it->set_value(value, has_value);
    } else {
        m_attributes.emplace_back(name, value, has_value);
    }
    invalidate_document_query_cache();
}

}  // namespace hps
