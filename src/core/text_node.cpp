#include "hps/core/text_node.hpp"

#include "hps/utils/string_utils.hpp"

namespace hps {

TextNode::TextNode(const std::string_view text) noexcept
    : Node(NodeType::Text),
      m_view(text) {}

NodeType TextNode::type() const noexcept {
    return NodeType::Text;
}

std::string_view TextNode::value() const noexcept {
    return m_materialized ? std::string_view(m_owned) : m_view;
}

std::string TextNode::text_content() const {
    return std::string(value());
}

std::string TextNode::trim() const {
    return std::string(trim_whitespace(value()));
}

bool TextNode::empty() const noexcept {
    return value().empty();
}

size_t TextNode::length() const noexcept {
    return value().size();
}

void TextNode::append_text(const std::string_view text) {
    // 首次追加：把零拷贝视图物化为可增长缓冲；其后直接 append。
    if (!m_materialized) {
        m_owned.assign(m_view);
        m_materialized = true;
        m_view         = {};
    }
    m_owned.append(text);
}
}  // namespace hps
