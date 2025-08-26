#include "tokenizer.hpp"

#include "utils/exception.hpp"

#include <unordered_set>

namespace hps {

Tokenizer::Tokenizer(std::string_view source)
    : m_source(source), m_pos(0), m_state(TokenizerState::Data) {}

std::optional<Token> Tokenizer::next_token() {
    while (has_more()) {
        switch (m_state) {
            case TokenizerState::Data:
                return consume_data_state();
            case TokenizerState::TagOpen:
                return consume_tag_open_state();
            case TokenizerState::TagName:
                return consume_tag_name_state();
            case TokenizerState::EndTagOpen:
                return consume_end_tag_open_state();
            case TokenizerState::EndTagName:
                return consume_end_tag_name_state();
            case TokenizerState::BeforeAttributeName:
                return consume_before_attribute_name_state();
            case TokenizerState::AttributeName:
                return consume_attribute_name_state();
            case TokenizerState::AfterAttributeName:
                return consume_after_attribute_name_state();
            case TokenizerState::BeforeAttributeValue:
                return consume_before_attribute_value_state();
            case TokenizerState::AttributeValueDoubleQuoted:
                return consume_attribute_value_double_quoted_state();
            case TokenizerState::AttributeValueSingleQuoted:
                return consume_attribute_value_single_quoted_state();
            case TokenizerState::AttributeValueUnquoted:
                return consume_attribute_value_unquoted_state();
            case TokenizerState::SelfClosingStartTag:
                return consume_self_closing_start_tag_state();
            case TokenizerState::CommentStart:
                return consume_comment_start_state();
            case TokenizerState::Comment:
                return consume_comment_state();
            case TokenizerState::CommentEndDash:
                return consume_comment_end_dash_state();
            case TokenizerState::CommentEnd:
                return consume_comment_end_state();
            case TokenizerState::DOCTYPE:
                return consume_doctype_state();
            case TokenizerState::DOCTYPEName:
                return consume_doctype_name_state();
            case TokenizerState::AfterDOCTYPEName:
                return consume_after_doctype_name_state();
            case TokenizerState::ScriptData:
                return consume_script_data_state();
            case TokenizerState::RAWTEXT:
                return consume_rawtext_state();
            case TokenizerState::RCDATA:
                return consume_rcdata_state();
            case TokenizerState::CharacterReference:
                return consume_character_reference_state();
            case TokenizerState::NamedCharacterReference:
                return consume_named_character_reference_state();
            case TokenizerState::NumericCharacterReference:
                return consume_numeric_character_reference_state();
            case TokenizerState::MarkupDeclarationOpen:
                return consume_markup_declaration_open_state();
            case TokenizerState::CDATASection:
                return consume_cdata_section_state();
            default:
                throw HPSException("Unknown tokenizer state");
        }
    }
}

std::vector<Token> Tokenizer::tokenize_all() {
    std::vector<Token> tokens;
    while (auto token = next_token()) {
        tokens.push_back(std::move(token.value()));
        if (token.value().is_done()) {
            break;
        }
    }
    return tokens;
}

bool Tokenizer::has_more() const noexcept {
    return m_pos < m_source.length();
}

size_t Tokenizer::position() const noexcept {
    return m_pos;
}

size_t Tokenizer::total_length() const noexcept {
    return m_source.length();
}

std::optional<Token> Tokenizer::consume_data_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_tag_open_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_tag_name_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_end_tag_open_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_end_tag_name_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_before_attribute_name_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_attribute_name_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_after_attribute_name_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_before_attribute_value_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_attribute_value_double_quoted_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_attribute_value_single_quoted_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_attribute_value_unquoted_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_self_closing_start_tag_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_comment_start_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_comment_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_comment_end_dash_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_comment_end_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_doctype_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_doctype_name_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_after_doctype_name_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_script_data_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_rawtext_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_rcdata_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_character_reference_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_named_character_reference_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_numeric_character_reference_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_markup_declaration_open_state() {
    return std::optional<Token>();
}

std::optional<Token> Tokenizer::consume_cdata_section_state() {
    return std::optional<Token>();
}

char Tokenizer::current_char() const noexcept {
    if (m_pos >= m_source.length()) {
        return '\0';
    }
    return m_source[m_pos];
}

char Tokenizer::peek_char(size_t offset) const noexcept {
    size_t peek_pos = m_pos + offset;
    if (peek_pos >= m_source.length()) {
        return '\0';
    }
    return m_source[peek_pos];
}

void Tokenizer::advance() noexcept {
    if (m_pos < m_source.length()) {
        m_pos++;
    }
}

void Tokenizer::skip_whitespace() noexcept {
    while (m_pos < m_source.length() && is_whitespace(m_source[m_pos])) {
        m_pos++;
    }
}

bool Tokenizer::starts_with(std::string_view s) const noexcept {
    if (m_pos + s.length() > m_source.length()) {
        return false;
    }
    return m_source.substr(m_pos, s.length()) == s;
}

bool Tokenizer::is_whitespace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
}

bool Tokenizer::is_alpha(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool Tokenizer::is_alnum(char c) noexcept {
    return is_alpha(c) || (c >= '0' && c <= '9');
}

char Tokenizer::to_lower(char c) noexcept {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
}

bool Tokenizer::is_void_element_name(std::string_view n) noexcept {
    static const std::unordered_set<std::string_view> void_elements = {"area",
                                                                       "base",
                                                                       "br",
                                                                       "col",
                                                                       "embed",
                                                                       "hr",
                                                                       "img",
                                                                       "input",
                                                                       "link",
                                                                       "meta",
                                                                       "param",
                                                                       "source",
                                                                       "track",
                                                                       "wbr"};

    return void_elements.find(n) != void_elements.end();
}

Token Tokenizer::create_start_tag_token() {}

Token Tokenizer::create_end_tag_token() {}

Token Tokenizer::create_text_token() {}

Token Tokenizer::create_comment_token() {}

Token Tokenizer::create_doctype_token() {}

Token Tokenizer::create_close_self_token() {}

Token Tokenizer::create_done_token() {}

}  // namespace hps