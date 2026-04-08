#include "hps/core/node.hpp"

#include "hps/core/comment_node.hpp"
#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/core/text_node.hpp"

#include <algorithm>
#include <utility>

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

const Document* Node::owner_document() const noexcept {
    const Node* current = this;
    while (current && current->m_parent) {
        current = current->m_parent;
    }
    return current ? current->as_document() : nullptr;
}

Document* Node::owner_document_mut() noexcept {
    return const_cast<Document*>(std::as_const(*this).owner_document());
}

void Node::invalidate_document_query_cache() noexcept {
    if (auto* document = owner_document_mut()) {
        document->invalidate_query_indexes();
    }
}

Node* Node::append_child(std::unique_ptr<Node> child) {
    if (!child) {
        return nullptr;
    }
    Node* inserted = child.get();
    child->set_parent(this);

    if (!m_children.empty()) {
        Node* last            = m_children.back().get();
        last->m_next_sibling  = child.get();
        child->m_prev_sibling = last;
    }

    m_children.push_back(std::move(child));
    return inserted;
}

Node* Node::insert_child_before(std::unique_ptr<Node> child, const Node* before) {
    if (!child) {
        return nullptr;
    }
    if (before == nullptr) {
        return append_child(std::move(child));
    }

    const auto it = std::ranges::find_if(
        m_children, [before](const std::unique_ptr<Node>& existing) { return existing.get() == before; });
    if (it == m_children.end()) {
        return append_child(std::move(child));
    }

    Node* inserted = child.get();
    child->set_parent(this);

    Node* next = it->get();
    Node* prev = next->m_prev_sibling;
    child->m_prev_sibling = prev;
    child->m_next_sibling = next;
    next->m_prev_sibling  = inserted;
    if (prev != nullptr) {
        prev->m_next_sibling = inserted;
    }

    m_children.insert(it, std::move(child));
    return inserted;
}

void Node::set_parent(Node* parent_node) noexcept {
    m_parent = parent_node;
}

std::vector<std::unique_ptr<Node>> Node::take_children() {
    for (auto& child : m_children) {
        child->m_parent       = nullptr;
        child->m_prev_sibling = nullptr;
        child->m_next_sibling = nullptr;
    }

    auto children = std::move(m_children);
    m_children.clear();
    return children;
}

}  // namespace hps
