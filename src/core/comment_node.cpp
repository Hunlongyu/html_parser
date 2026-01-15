#include "hps/core/comment_node.hpp"

#include "hps/utils/string_utils.hpp"

namespace hps {

CommentNode::CommentNode(const std::string_view comment)
    : Node(NodeType::Comment),
      m_comment(comment) {}

NodeType CommentNode::type() const noexcept {
    return NodeType::Comment;
}

const std::string& CommentNode::value() const {
    return m_comment;
}

std::string CommentNode::text_content() const {
    return m_comment;
}

const std::string& CommentNode::comment() const {
    return m_comment;
}

std::string CommentNode::trim() const {
    const auto trimmed = trim_whitespace(m_comment);
    return std::string(trimmed);
}

bool CommentNode::empty() const noexcept {
    return m_comment.empty();
}

size_t CommentNode::length() const noexcept {
    return m_comment.length();
}
}  // namespace hps
