#include "hps/parsing/token.hpp"

namespace hps {

Token::Token(TokenType type, std::string_view name, std::string_view value) noexcept : m_type(type), m_name(name), m_value(value) {}

TokenType Token::type() const noexcept {
    return m_type;
}

std::string_view Token::name() const noexcept {
    return m_name;
}

std::string_view Token::value() const noexcept {
    return m_value;
}

void Token::set_type(TokenType type) noexcept {
    m_type = type;
}

void Token::add_attr(const TokenAttribute& attr) {
    m_attrs.push_back(attr);
}

void Token::add_attr(std::string_view name, std::string_view value, bool has_value) {
    m_attrs.emplace_back(name, value, has_value);
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

bool Token::is_force_quirks() const noexcept {
    return m_type == TokenType::FORCE_QUIRKS;
}

bool Token::is_done() const noexcept {
    return m_type == TokenType::DONE;
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

bool Token::is_tag(std::string_view name) const noexcept {
    return (m_type == TokenType::OPEN || m_type == TokenType::CLOSE || m_type == TokenType::CLOSE_SELF) && m_name == name;
}

}  // namespace hps