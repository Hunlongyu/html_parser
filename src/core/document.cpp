#include "hps/core/document.hpp"

#include "hps/query/element_query.hpp"

namespace hps {
Document::Document(std::string html_content) : Node(NodeType::Document), m_html_source(std::move(html_content)) {}

NodeType Document::node_type() const {
    return {};
}

std::string Document::node_name() const {
    return {};
}

std::string Document::node_value() const {
    return {};
}

std::string Document::text_content() const {
    return {};
}

std::string Document::title() const {
    return {};
}

std::string Document::charset() const {
    return {};
}

std::string Document::source_html() const {
    return {};
}

std::string Document::get_meta_content(std::string_view name) const {
    return {};
}

std::string Document::get_meta_property(std::string_view property) const {
    return {};
}

std::vector<std::string> Document::get_all_links() const {
    return {};
}

std::vector<std::string> Document::get_all_images() const {
    return {};
}

std::shared_ptr<const Element> Document::root() const {
    return nullptr;
}

std::shared_ptr<const Element> Document::html() const {
    return nullptr;
}

std::shared_ptr<const Element> Document::document() const {
    return nullptr;
}

std::shared_ptr<const Element> Document::querySelector(std::string_view selector) const {
    return nullptr;
}

std::vector<std::shared_ptr<const Element>> Document::querySelectorAll(std::string_view selector) const {
    return {};
}

std::shared_ptr<const Element> Document::get_element_by_id(std::string_view id) const {
    return nullptr;
}

std::vector<std::shared_ptr<const Element>> Document::get_elements_by_tag_name(std::string_view tag_name) const {
    return {};
}

std::vector<std::shared_ptr<const Element>> Document::get_elements_by_class_name(std::string_view class_name) const {
    return {};
}

ElementQuery Document::css(std::string_view selector) const {
    return {};
}

ElementQuery Document::xpath(std::string_view expression) const {
    return {};
}

void Document::add_child(std::unique_ptr<Node> child) {
    Node::add_child(std::move(child));
}
}  // namespace hps