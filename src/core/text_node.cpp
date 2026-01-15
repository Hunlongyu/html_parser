#include "hps/core/text_node.hpp"

#include "hps/core/element.hpp"
#include "hps/utils/string_utils.hpp"

namespace hps {

TextNode::TextNode(const std::string_view text) noexcept
    : Node(NodeType::Text),
      m_text(text) {}

NodeType TextNode::type() const noexcept {
    return NodeType::Text;
}

std::string TextNode::name() const {
    const auto parent_node = parent();
    if (parent_node && parent_node->is_element()) {
        const auto parent_element = parent_node->as_element();
        return parent_element->tag_name();
    }
    return {};
}

const std::string& TextNode::value() const {
    return m_text;
}

std::string TextNode::text_content() const {
    return m_text;
}

const std::string& TextNode::text() const {
    return m_text;
}

std::string TextNode::trim() const {
    const auto trimmed = trim_whitespace(m_text);
    return std::string(trimmed);
}

bool TextNode::empty() const noexcept {
    return m_text.empty();
}

size_t TextNode::length() const noexcept {
    return m_text.length();
}
}  // namespace hps
