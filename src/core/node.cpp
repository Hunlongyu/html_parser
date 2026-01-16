#include "hps/core/node.hpp"

#include "hps/core/comment_node.hpp"
#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/core/text_node.hpp"

#include <algorithm>

namespace hps {
Node::Node(const NodeType type) noexcept
    : m_type(type) {}

const Node* Node::parent() const noexcept {
    return m_parent;
}

std::vector<const Node*> Node::children() const noexcept {
    std::vector<const Node*> result;
    result.reserve(m_children.size());
    for (const auto& child : m_children) {
        result.push_back(child.get());
    }
    return result;
}

const Node* Node::first_child() const noexcept {
    if (m_children.empty()) {
        return nullptr;
    }
    return m_children.front().get();
}

const Node* Node::last_child() const noexcept {
    if (m_children.empty()) {
        return nullptr;
    }
    return m_children.back().get();
}

Node* Node::last_child_mut() noexcept {
    if (m_children.empty()) {
        return nullptr;
    }
    return m_children.back().get();
}

const Node* Node::previous_sibling() const noexcept {
    return m_prev_sibling;
}

const Node* Node::next_sibling() const noexcept {
    return m_next_sibling;
}

std::vector<const Node*> Node::siblings() const noexcept {
    std::vector<const Node*> result;
    if (!m_parent) {
        return result;
    }
    const auto& parent_children = m_parent->m_children;
    if (!parent_children.empty()) {
        result.reserve(parent_children.size() - 1);
    }
    for (const auto& child : parent_children) {
        if (child.get() != this) {
            result.push_back(child.get());
        }
    }
    return result;
}

const Document* Node::as_document() const noexcept {
    return dynamic_cast<const Document*>(this);
}

const Element* Node::as_element() const noexcept {
    return dynamic_cast<const Element*>(this);
}

const TextNode* Node::as_text() const noexcept {
    return dynamic_cast<const TextNode*>(this);
}

const CommentNode* Node::as_comment() const noexcept {
    return dynamic_cast<const CommentNode*>(this);
}

void Node::append_child(std::unique_ptr<Node> child) {
    if (!child) {
        return;
    }
    child->set_parent(this);

    if (!m_children.empty()) {
        Node* last            = m_children.back().get();
        last->m_next_sibling  = child.get();
        child->m_prev_sibling = last;
    }

    m_children.push_back(std::move(child));
}

void Node::set_parent(Node* parent_node) noexcept {
    m_parent = parent_node;
}

}  // namespace hps