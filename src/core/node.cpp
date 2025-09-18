#include "hps/core/node.hpp"

#include "hps/core/comment_node.hpp"
#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/core/text_node.hpp"

#include <algorithm>

namespace hps {

Node::Node(const NodeType type) noexcept
    : m_type(type) {}

std::shared_ptr<const Node> Node::parent() const noexcept {
    return m_parent.lock();
}

std::vector<std::shared_ptr<const Node>> Node::children() const noexcept {
    std::vector<std::shared_ptr<const Node>> const_children;
    const_children.reserve(m_children.size());
    for (const auto& child_ptr : m_children) {
        const_children.push_back(child_ptr);
    }
    return const_children;
}

std::shared_ptr<const Node> Node::first_child() const noexcept {
    if (m_children.empty()) {
        return nullptr;
    }
    return m_children.front();
}

std::shared_ptr<const Node> Node::last_child() const noexcept {
    if (m_children.empty()) {
        return nullptr;
    }
    return m_children.back();
}

std::shared_ptr<const Node> Node::previous_sibling() const noexcept {
    if (m_parent.expired()) {
        return nullptr;
    }
    const auto shared_parent = m_parent.lock();
    if (!shared_parent) {
        return nullptr;
    }
    const auto& siblings = shared_parent->m_children;

    auto it = std::ranges::find_if(siblings, [this](const std::shared_ptr<Node>& c) { return c.get() == this; });

    if (it != siblings.begin() && it != siblings.end()) {
        return *(--it);
    }
    return nullptr;
}

std::shared_ptr<const Node> Node::next_sibling() const noexcept {
    if (m_parent.expired()) {
        return nullptr;
    }
    const auto shared_parent = m_parent.lock();
    if (!shared_parent) {
        return nullptr;
    }
    const auto& siblings = shared_parent->m_children;

    auto it = std::ranges::find_if(siblings, [this](const std::shared_ptr<Node>& c) { return c.get() == this; });

    if (it != siblings.end() && std::next(it) != siblings.end()) {
        return *(++it);
    }
    return nullptr;
}

void Node::append_child(const std::shared_ptr<Node>& child) {
    if (!child) {
        return;
    }
    child->remove_from_parent();
    m_children.push_back(child);
    child->set_parent(shared_from_this());
}

void Node::insert_before(const std::shared_ptr<Node>& new_child, const Node* ref_child) {
    if (!new_child || !ref_child) {
        return;
    }
    new_child->remove_from_parent();
    const auto it = std::ranges::find_if(m_children, [ref_child](const std::shared_ptr<Node>& c) { return c.get() == ref_child; });
    if (it != m_children.end()) {
        m_children.insert(it, new_child);
        new_child->set_parent(shared_from_this());
    } else {
        append_child(new_child);
    }
}

void Node::remove_child(const Node* child) {
    if (!child) {
        return;
    }
    const auto it = std::ranges::remove_if(m_children, [child](const std::shared_ptr<Node>& c) { return c.get() == child; }).begin();
    if (it != m_children.end()) {
        (*it)->m_parent.reset();
        m_children.erase(it, m_children.end());
    }
}

void Node::set_parent(const std::shared_ptr<Node>& parent_node) noexcept {
    m_parent = parent_node;
}

void Node::remove_from_parent() const noexcept {
    if (auto parent_ptr = m_parent.lock()) {
        parent_ptr->remove_child(this);
    }
}

std::shared_ptr<const Document> Node::as_document() const noexcept {
    return std::dynamic_pointer_cast<const Document>(shared_from_this());
}

std::shared_ptr<const Element> Node::as_element() const noexcept {
    return std::dynamic_pointer_cast<const Element>(shared_from_this());
}

std::shared_ptr<const TextNode> Node::as_text() const noexcept {
    return std::dynamic_pointer_cast<const TextNode>(shared_from_this());
}

std::shared_ptr<const CommentNode> Node::as_comment() const noexcept {
    return std::dynamic_pointer_cast<const CommentNode>(shared_from_this());
}

}  // namespace hps