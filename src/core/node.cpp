#include "hps/core/node.hpp"

#include "hps/core/comment_node.hpp"
#include "hps/core/doctype_node.hpp"
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
    for (const Node* child = m_first_child; child != nullptr; child = child->m_next_sibling) {
        result.push_back(child);
    }
    return result;
}

const Node* Node::first_child() const noexcept {
    return m_first_child;
}

const Node* Node::last_child() const noexcept {
    return m_last_child;
}

Node* Node::last_child_mut() noexcept {
    return m_last_child;
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
    for (const Node* child = m_parent->m_first_child; child != nullptr; child = child->m_next_sibling) {
        if (child != this) {
            result.push_back(child);
        }
    }
    return result;
}

// 节点层级是封闭的，且每个派生类在构造时写入了对应的 m_type，
// 因此用类型标签 + static_cast 替代 dynamic_cast，避免 RTTI 开销（热路径）。
const Document* Node::as_document() const noexcept {
    return m_type == NodeType::Document ? static_cast<const Document*>(this) : nullptr;
}

const Element* Node::as_element() const noexcept {
    return m_type == NodeType::Element ? static_cast<const Element*>(this) : nullptr;
}

const TextNode* Node::as_text() const noexcept {
    return m_type == NodeType::Text ? static_cast<const TextNode*>(this) : nullptr;
}

const CommentNode* Node::as_comment() const noexcept {
    return m_type == NodeType::Comment ? static_cast<const CommentNode*>(this) : nullptr;
}

const DoctypeNode* Node::as_doctype() const noexcept {
    return m_type == NodeType::Doctype ? static_cast<const DoctypeNode*>(this) : nullptr;
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

Node* Node::append_child(Node* child) {
    if (child == nullptr) {
        return nullptr;
    }
    child->m_parent       = this;
    child->m_prev_sibling = m_last_child;
    child->m_next_sibling = nullptr;
    if (m_last_child != nullptr) {
        m_last_child->m_next_sibling = child;
    } else {
        m_first_child = child;
    }
    m_last_child = child;
    return child;
}

Node* Node::insert_child_before(Node* child, const Node* before) {
    if (child == nullptr) {
        return nullptr;
    }
    if (before == nullptr) {
        return append_child(child);
    }
    auto* next = const_cast<Node*>(before);
    if (next->m_parent != this) {
        return append_child(child);  // before 不是本节点的子节点：退化为追加
    }

    Node* prev            = next->m_prev_sibling;
    child->m_parent       = this;
    child->m_prev_sibling = prev;
    child->m_next_sibling = next;
    next->m_prev_sibling  = child;
    if (prev != nullptr) {
        prev->m_next_sibling = child;
    } else {
        m_first_child = child;
    }
    return child;
}

void Node::set_parent(Node* parent_node) noexcept {
    m_parent = parent_node;
}

void Node::remove_child(Node* child) {
    if (child == nullptr || child->m_parent != this) {
        return;
    }
    Node* prev = child->m_prev_sibling;
    Node* next = child->m_next_sibling;
    if (prev != nullptr) {
        prev->m_next_sibling = next;
    } else {
        m_first_child = next;
    }
    if (next != nullptr) {
        next->m_prev_sibling = prev;
    } else {
        m_last_child = prev;
    }
    child->m_parent       = nullptr;
    child->m_prev_sibling = nullptr;
    child->m_next_sibling = nullptr;
}

std::vector<Node*> Node::take_children() {
    std::vector<Node*> result;
    for (Node* child = m_first_child; child != nullptr;) {
        Node* next            = child->m_next_sibling;
        child->m_parent       = nullptr;
        child->m_prev_sibling = nullptr;
        child->m_next_sibling = nullptr;
        result.push_back(child);
        child = next;
    }
    m_first_child = nullptr;
    m_last_child  = nullptr;
    return result;
}

}  // namespace hps
