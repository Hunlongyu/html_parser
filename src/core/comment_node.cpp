#include "hps/core/comment_node.hpp"

namespace hps {

CommentNode::CommentNode(std::string_view comment) noexcept : Node(NodeType::Comment), m_comment(comment) {}

NodeType CommentNode::type() {
    return NodeType::Comment;
}

std::string CommentNode::name() const {
    return {};
}

std::string CommentNode::value() const {
    return m_comment;
}

std::string CommentNode::text_content() const {
    return m_comment;
}

std::string CommentNode::comment() const {
    return m_comment;
}

std::string CommentNode::normalized_comment() const {
    if (m_comment.empty()) {
        return m_comment;
    }
    size_t start = 0;
    while (start < m_comment.size() && std::isspace(static_cast<unsigned char>(m_comment[start]))) {
        ++start;
    }
    if (start == m_comment.size()) {
        return {};
    }
    size_t end = m_comment.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(m_comment[end]))) {
        --end;
    }
    return {m_comment.data() + start, end - start + 1};
}

bool CommentNode::empty() const noexcept {
    return m_comment.empty();
}

size_t CommentNode::length() const noexcept {
    return m_comment.length();
}
}  // namespace hps