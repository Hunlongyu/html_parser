#include "hps/parsing/token.hpp"

namespace hps {

Token::Token(const TokenType type, const std::string_view name, const std::string_view value) noexcept
    : m_type(type),
      m_name(name),
      m_value(value) {}

Token::Token(Token&& other) noexcept
    : m_type(other.m_type),
      m_name(std::move(other.m_name)),
      m_value(other.m_value),
      m_value_owned(std::move(other.m_value_owned)),
      m_doctype_public_id(std::move(other.m_doctype_public_id)),
      m_doctype_system_id(std::move(other.m_doctype_system_id)),
      m_doctype_force_quirks(other.m_doctype_force_quirks),
      m_attrs(std::move(other.m_attrs)) {
    other.m_value = {};
    other.m_doctype_force_quirks = false;
}

Token& Token::operator=(Token&& other) noexcept {
    if (this != &other) {
        m_type                 = other.m_type;
        m_name                 = std::move(other.m_name);
        m_value                = other.m_value;
        m_value_owned          = std::move(other.m_value_owned);
        m_doctype_public_id    = std::move(other.m_doctype_public_id);
        m_doctype_system_id    = std::move(other.m_doctype_system_id);
        m_doctype_force_quirks = other.m_doctype_force_quirks;
        m_attrs                = std::move(other.m_attrs);
        other.m_value          = {};
        other.m_doctype_force_quirks = false;
    }
    return *this;
}

TokenType Token::type() const noexcept {
    return m_type;
}

std::string_view Token::name() const noexcept {
    return m_name;
}

std::string_view Token::value() const noexcept {
    if (!m_value_owned.empty()) {
        return m_value_owned;
    }
    return m_value;
}

std::string_view Token::doctype_public_id() const noexcept {
    return m_doctype_public_id;
}

std::string_view Token::doctype_system_id() const noexcept {
    return m_doctype_system_id;
}

bool Token::doctype_force_quirks() const noexcept {
    return m_doctype_force_quirks;
}

void Token::set_owned_value(std::string value) {
    m_value_owned = std::move(value);
}

void Token::set_doctype_identifiers(const std::string_view public_id, const std::string_view system_id) {
    m_doctype_public_id = public_id;
    m_doctype_system_id = system_id;
}

void Token::set_doctype_force_quirks(const bool force_quirks) noexcept {
    m_doctype_force_quirks = force_quirks;
}

void Token::set_type(const TokenType type) noexcept {
    m_type = type;
}

void Token::add_attr(const TokenAttribute& attr) {
    m_attrs.push_back(attr);
}

void Token::add_attr(TokenAttribute&& attr) {
    m_attrs.push_back(std::move(attr));
}

void Token::add_attr(std::string_view name, std::string_view value, bool has_value) {
    m_attrs.emplace_back(std::string(name), value, has_value);
}

const std::vector<TokenAttribute>& Token::attrs() const noexcept {
    return m_attrs;
}

bool Token::is_open() const noexcept {
    return m_type == TokenType::OPEN;
}

bool Token::is_close() const noexcept {
    return m_type == TokenType::CLOSE;
}

bool Token::is_close_self() const noexcept {
    return m_type == TokenType::CLOSE_SELF;
}

bool Token::is_done() const noexcept {
    return m_type == TokenType::DONE;
}

bool Token::is_force_quirks() const noexcept {
    return m_type == TokenType::FORCE_QUIRKS;
}

bool Token::is_text() const noexcept {
    return m_type == TokenType::TEXT;
}

bool Token::is_comment() const noexcept {
    return m_type == TokenType::COMMENT;
}

bool Token::is_doctype() const noexcept {
    return m_type == TokenType::DOCTYPE;
}

bool Token::is_tag(const std::string_view name) const noexcept {
    return (m_type == TokenType::OPEN || m_type == TokenType::CLOSE || m_type == TokenType::CLOSE_SELF) && m_name == name;
}

}  // namespace hps
