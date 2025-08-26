#include "core/node.hpp"

namespace hps {

Node::Node(NodeType type) : m_type(type) {}

Node::~Node() {}

NodeType Node::node_type() const {
    return m_type;
}

std::string_view Node::node_name() const {
    return {};
}

std::string_view Node::node_value() const {
    return {};
}

std::string_view Node::text_content() const {
    return {};
}

const Node* Node::parent() const noexcept {
    return nullptr;
}

const Node* Node::first_child() const noexcept {
    return nullptr;
}

const Node* Node::last_child() const noexcept {
    return nullptr;
}

const Node* Node::previous_sibling() const noexcept {
    return nullptr;
}

const Node* Node::next_sibling() const noexcept {
    return nullptr;
}

const std::vector<std::unique_ptr<Node>>& Node::children() const noexcept {
    return {};
}

bool Node::has_children() const noexcept {
    return false;
}

size_t Node::child_count() const noexcept {
    return 0;
}

const Node* Node::child_at(size_t index) const noexcept {
    return nullptr;
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