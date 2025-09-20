#include "hps/core/text_node.hpp"

#include "hps/core/element.hpp"

namespace hps {

TextNode::TextNode(const std::string_view text) noexcept
    : Node(NodeType::Text),
      m_text(text) {}

NodeType TextNode::type() const noexcept {
    return NodeType::Text;
}

std::string TextNode::name() const {
    auto parent_node = parent();
    if (parent_node && parent_node->is_element()) {
        const auto parent_element = std::static_pointer_cast<const Element>(parent_node);
        return parent_element->tag_name();
    }
    return {};
}

std::string TextNode::value() const {
    return m_text;
}

std::string TextNode::text_content() const {
    return m_text;
}

std::string TextNode::text() const {
    return m_text;
}

std::string TextNode::trim() const {
    if (m_text.empty()) {
        return m_text;
    }
    size_t start = 0;
    while (start < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[start]))) {
        ++start;
    }
    if (start == m_text.size()) {
        return {};
    }
    size_t end = m_text.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(m_text[end]))) {
        --end;
    }
    return {m_text.data() + start, end - start + 1};
}

bool TextNode::empty() const noexcept {
    return m_text.empty();
}

size_t TextNode::length() const noexcept {
    return m_text.length();
}
}  // namespace hps