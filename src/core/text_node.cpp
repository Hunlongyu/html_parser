#include "core/text_node.hpp"

namespace hps {

TextNode::TextNode(std::string_view text) noexcept : Node(NodeType::Text), m_text(text) {}

NodeType TextNode::node_type() const {
    return NodeType::Text;
}

std::string_view TextNode::node_name() const {
    return {};
}

std::string_view TextNode::node_value() const {
    return m_text;
}

std::string_view TextNode::text_content() const {
    return m_text;
}

std::string_view TextNode::text() const {
    return m_text;
}

bool TextNode::empty() const noexcept {
    return m_text.empty();
}

size_t TextNode::length() const noexcept {
    return m_text.length();
}
}  // namespace hps