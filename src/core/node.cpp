#include "hps/core/node.hpp"

namespace hps {

Node::Node(const NodeType type) : m_type(type), m_parent(nullptr) {}

Node::Node(const NodeType type, Node* parent) : m_type(type), m_parent(parent) {}

NodeType Node::node_type() const {
    return m_type;
}

const Node* Node::parent() const noexcept {
    return m_parent;
}

const Node* Node::first_child() const noexcept {
    return m_children.empty() ? nullptr : m_children.front().get();
}

const Node* Node::last_child() const noexcept {
    return m_children.empty() ? nullptr : m_children.back().get();
}

const Node* Node::previous_sibling() const noexcept {
    if (!m_parent) {
        return nullptr;
    }
    const auto& siblings = m_parent->m_children;
    for (auto it = siblings.begin(); it != siblings.end(); ++it) {
        if (it->get() == this) {
            return it == siblings.begin() ? nullptr : std::prev(it)->get();
        }
    }
    return nullptr;
}

const Node* Node::next_sibling() const noexcept {
    if (!m_parent) {
        return nullptr;
    }
    const auto& siblings = m_parent->m_children;
    for (auto it = siblings.begin(); it != siblings.end(); ++it) {
        if (it->get() == this) {
            return std::next(it) == siblings.end() ? nullptr : std::next(it)->get();
        }
    }
    return nullptr;
}

const std::vector<std::unique_ptr<Node>>& Node::children() const noexcept {
    return m_children;
}

bool Node::has_children() const noexcept {
    return !m_children.empty();
}

size_t Node::child_count() const noexcept {
    return m_children.size();
}

const Node* Node::child_at(size_t index) const noexcept {
    return index < m_children.size() ? m_children[index].get() : nullptr;
}

bool Node::is_element() const noexcept {
    return node_type() == NodeType::Element;
}

bool Node::is_text() const noexcept {
    return node_type() == NodeType::Text;
}

bool Node::is_comment() const noexcept {
    return node_type() == NodeType::Comment;
}

bool Node::is_document() const noexcept {
    return node_type() == NodeType::Document;
}

void Node::set_parent(Node* parent) {
    m_parent = parent;
}

void Node::add_child(std::unique_ptr<Node> child) {
    if (child) {
        child->set_parent(this);
        m_children.push_back(std::move(child));
    }
}

}  // namespace hps