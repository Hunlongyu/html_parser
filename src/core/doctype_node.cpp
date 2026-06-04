#include "hps/core/doctype_node.hpp"

namespace hps {

DoctypeNode::DoctypeNode(
    const std::string_view name,
    const std::string_view public_id,
    const std::string_view system_id,
    const bool has_identifiers)
    : Node(NodeType::Doctype),
      m_name(name),
      m_public_id(public_id),
      m_system_id(system_id),
      m_has_identifiers(has_identifiers) {}

NodeType DoctypeNode::type() const noexcept {
    return NodeType::Doctype;
}

const std::string& DoctypeNode::name() const noexcept {
    return m_name;
}

const std::string& DoctypeNode::public_id() const noexcept {
    return m_public_id;
}

const std::string& DoctypeNode::system_id() const noexcept {
    return m_system_id;
}

bool DoctypeNode::has_identifiers() const noexcept {
    return m_has_identifiers;
}

std::string DoctypeNode::text_content() const {
    return {};
}

}  // namespace hps
