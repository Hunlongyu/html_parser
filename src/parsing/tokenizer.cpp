#include "hps/parsing/tokenizer.hpp"

#include "hps/utils/exception.hpp"
#include "hps/utils/string_utils.hpp"

namespace hps {

Tokenizer::Tokenizer(const std::string_view source, const Options& options)
    : m_source(source),
      m_pos(0),
      m_state(TokenizerState::Data),
      m_options(options) {}

std::optional<Token> Tokenizer::next_token() {
    while (has_more()) {
        switch (m_state) {
            case TokenizerState::Data:
                if (auto token = consume_data_state())
                    return token;
                break;
            case TokenizerState::TagOpen:
                if (auto token = consume_tag_open_state())
                    return token;
                break;
            case TokenizerState::TagName:
                if (auto token = consume_tag_name_state())
                    return token;
                break;
            case TokenizerState::EndTagOpen:
                if (auto token = consume_end_tag_open_state())
                    return token;
                break;
            case TokenizerState::EndTagName:
                if (auto token = consume_end_tag_name_state())
                    return token;
                break;
            case TokenizerState::BeforeAttributeName:
                if (auto token = consume_before_attribute_name_state())
                    return token;
                break;
            case TokenizerState::AttributeName:
                if (auto token = consume_attribute_name_state())
                    return token;
                break;
            case TokenizerState::AfterAttributeName:
                if (auto token = consume_after_attribute_name_state())
                    return token;
                break;
            case TokenizerState::BeforeAttributeValue:
                if (auto token = consume_before_attribute_value_state())
                    return token;
                break;
            case TokenizerState::AttributeValueDoubleQuoted:
                if (auto token = consume_attribute_value_double_quoted_state())
                    return token;
                break;
            case TokenizerState::AttributeValueSingleQuoted:
                if (auto token = consume_attribute_value_single_quoted_state())
                    return token;
                break;
            case TokenizerState::AttributeValueUnquoted:
                if (auto token = consume_attribute_value_unquoted_state())
                    return token;
                break;
            case TokenizerState::SelfClosingStartTag:
                if (auto token = consume_self_closing_start_tag_state())
                    return token;
                break;
            case TokenizerState::Comment:
                if (auto token = consume_comment_state())
                    return token;
                break;
            case TokenizerState::DOCTYPE:
                if (auto token = consume_doctype_state())
                    return token;
                break;
            case TokenizerState::ScriptData:
                if (auto token = consume_script_data_state())
                    return token;
                break;
            case TokenizerState::RAWTEXT:
                if (auto token = consume_rawtext_state())
                    return token;
                break;
            case TokenizerState::RCDATA:
                if (auto token = consume_rcdata_state())
                    return token;
                break;
        }
    }
    return create_done_token();
}

std::vector<Token> Tokenizer::tokenize_all() {
    std::vector<Token> tokens;
    while (auto token = next_token()) {
        if (token->is_done()) {
            break;
        }
        tokens.push_back(std::move(*token));
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

const std::vector<HPSError>& Tokenizer::get_errors() const noexcept {
    return m_errors;
}

std::optional<Token> Tokenizer::consume_data_state() {
    if (current_char() == '<') {
        advance();
        m_state = TokenizerState::TagOpen;
        return {};
    }

    const size_t start = m_pos;
    while (has_more() && current_char() != '<') {
        advance();
    }
    if (start < m_pos) {
        const std::string_view data              = m_source.substr(start, m_pos - start);
        bool                   is_all_whitespace = true;
        for (const char c : data) {
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\f') {
                is_all_whitespace = false;
                break;
            }
        }
        if (!is_all_whitespace) {
            return create_text_token(data);
        }
    }
    return {};
}

std::optional<Token> Tokenizer::consume_tag_open_state() {
    if (current_char() == '!') {
        advance();
        if (starts_with("DOCTYPE") || starts_with("doctype")) {
            m_state = TokenizerState::DOCTYPE;
        } else if (starts_with("[CDATA[")) {
            for (int i = 0; i < 7 && has_more(); i++) {
                advance();
            }
            std::string cdata_content;
            while (has_more()) {
                if (starts_with("]]")) {
                    advance();
                    advance();
                    if (has_more() && current_char() == '>') {
                        advance();
                    }
                    break;
                }
                cdata_content += current_char();
                advance();
            }
            m_state = TokenizerState::Data;
            return create_text_token(cdata_content);
        } else {
            m_state = TokenizerState::Comment;
        }
    } else if (current_char() == '/') {
        advance();
        m_state = TokenizerState::EndTagOpen;
    } else if (is_alpha(current_char())) {
        m_token_builder.reset();
        m_state = TokenizerState::TagName;
    } else if (current_char() == '?') {
        advance();
        while (has_more() && current_char() != '>') {
            advance();
        }
        if (current_char() == '>') {
            advance();
        }
        m_state = TokenizerState::Data;
    } else {
        m_state = TokenizerState::Data;
        return create_text_token("<");
    }
    return {};
}

std::optional<Token> Tokenizer::consume_tag_name_state() {
    while (has_more() && is_alnum(current_char())) {
        if (m_options.preserve_case) {
            m_token_builder.tag_name += current_char();
        } else {
            m_token_builder.tag_name += to_lower(current_char());
        }
        advance();
    }
    if (is_whitespace(current_char())) {
        skip_whitespace();
        m_state = TokenizerState::BeforeAttributeName;
    } else if (current_char() == '>') {
        advance();
        m_state = TokenizerState::Data;
        return create_start_tag_token();
    } else if (current_char() == '/') {
        advance();
        m_state = TokenizerState::SelfClosingStartTag;
    } else if (current_char() == '\0') {
        handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in tag name");
        return {};
    }
    return {};
}

std::optional<Token> Tokenizer::consume_end_tag_open_state() {
    if (is_alpha(current_char())) {
        m_end_tag.clear();
        m_state = TokenizerState::EndTagName;
    } else if (current_char() == '>') {
        handle_parse_error(ErrorCode::InvalidToken, "Empty end tag");
        advance();
        m_state = TokenizerState::Data;
    } else {
        handle_parse_error(ErrorCode::InvalidToken, "Invalid character after </");
        m_state = TokenizerState::Data;
    }
    return {};
}

std::optional<Token> Tokenizer::consume_end_tag_name_state() {
    while (has_more() && is_alnum(current_char())) {
        if (m_options.preserve_case) {
            m_end_tag += current_char();
        } else {
            m_end_tag += to_lower(current_char());
        }
        advance();
    }

    if (is_whitespace(current_char())) {
        skip_whitespace();
    }

    if (current_char() == '>') {
        advance();
        m_state = TokenizerState::Data;
        return create_end_tag_token();
    }
    if (current_char() == '\0') {
        handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in end tag name");
        return {};
    }
    handle_parse_error(ErrorCode::InvalidToken, "Invalid character in end tag");
    while (has_more() && current_char() != '>') {
        advance();
    }
    if (current_char() == '>') {
        advance();
    }
    m_state = TokenizerState::Data;
    return create_end_tag_token();
}

std::optional<Token> Tokenizer::consume_before_attribute_name_state() {
    if (is_whitespace(current_char())) {
        skip_whitespace();
        return {};
    }

    if (current_char() == '>') {
        advance();
        m_state = TokenizerState::Data;
        return create_start_tag_token();
    }

    if (current_char() == '/') {
        advance();
        m_state = TokenizerState::SelfClosingStartTag;
        return {};
    }

    if (current_char() == '\0') {
        handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF before attribute name");
        return {};
    }

    if (current_char() == '\'' || current_char() == '"' || current_char() == '=') {
        handle_parse_error(ErrorCode::InvalidToken, "Unexpected character before attribute name");
        advance();
        return {};
    }
    m_token_builder.attr_name.clear();
    m_token_builder.attr_value.clear();
    m_state = TokenizerState::AttributeName;
    return {};
}

std::optional<Token> Tokenizer::consume_attribute_name_state() {
    while (has_more() && is_alnum(current_char()) || current_char() == '-' || current_char() == '_') {
        if (m_options.preserve_case) {
            m_token_builder.attr_name += current_char();
        } else {
            m_token_builder.attr_name += to_lower(current_char());
        }
        advance();
    }

    if (is_whitespace(current_char())) {
        skip_whitespace();
        m_state = TokenizerState::AfterAttributeName;
    } else if (current_char() == '=') {
        advance();
        m_state = TokenizerState::BeforeAttributeValue;
    } else if (current_char() == '>') {
        m_token_builder.finish_current_attribute();
        advance();
        m_state = TokenizerState::Data;
        return create_start_tag_token();
    } else if (current_char() == '/') {
        m_token_builder.finish_current_attribute();
        advance();
        m_state = TokenizerState::SelfClosingStartTag;
    } else if (current_char() == '\0') {
        handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in attribute name");
        return {};
    } else {
        m_token_builder.finish_current_attribute();
        m_state = TokenizerState::BeforeAttributeName;
    }
    return {};
}

std::optional<Token> Tokenizer::consume_after_attribute_name_state() {
    if (is_whitespace(current_char())) {
        skip_whitespace();
    } else if (current_char() == '=') {
        advance();
        m_state = TokenizerState::BeforeAttributeValue;
    } else if (current_char() == '>') {
        m_token_builder.finish_current_attribute();
        advance();
        m_state = TokenizerState::Data;
        return create_start_tag_token();
    } else if (current_char() == '/') {
        m_token_builder.finish_current_attribute();
        advance();
        m_state = TokenizerState::SelfClosingStartTag;
    } else if (current_char() == '\0') {
        handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF after attribute name");
        return {};
    } else {
        m_token_builder.finish_current_attribute();
        m_token_builder.attr_name.clear();
        m_state = TokenizerState::AttributeName;
    }
    return {};
}

std::optional<Token> Tokenizer::consume_before_attribute_value_state() {
    if (is_whitespace(current_char())) {
        skip_whitespace();
    } else if (current_char() == '"') {
        advance();
        m_state = TokenizerState::AttributeValueDoubleQuoted;
    } else if (current_char() == '\'') {
        advance();
        m_state = TokenizerState::AttributeValueSingleQuoted;
    } else if (current_char() == '>') {
        handle_parse_error(ErrorCode::InvalidToken, "Missing attribute value");
        m_token_builder.finish_current_attribute();
        advance();
        m_state = TokenizerState::Data;
        return create_start_tag_token();
    } else {
        m_token_builder.attr_value += current_char();
        advance();
        m_state = TokenizerState::AttributeValueUnquoted;
    }
    return {};
}

std::optional<Token> Tokenizer::consume_attribute_value_double_quoted_state() {
    if (current_char() == '"') {
        m_token_builder.finish_current_attribute();
        advance();
        m_state = TokenizerState::BeforeAttributeName;
    } else if (current_char() == '\0') {
        handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in attribute value");
        return {};
    } else {
        m_token_builder.attr_value += current_char();
        advance();
    }
    return {};
}

std::optional<Token> Tokenizer::consume_attribute_value_single_quoted_state() {
    if (current_char() == '\'') {
        m_token_builder.finish_current_attribute();
        advance();
        m_state = TokenizerState::BeforeAttributeName;
    } else if (current_char() == '\0') {
        handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in attribute value");
        return {};
    } else {
        m_token_builder.attr_value += current_char();
        advance();
    }
    return {};
}

std::optional<Token> Tokenizer::consume_attribute_value_unquoted_state() {
    if (is_whitespace(current_char())) {
        m_token_builder.finish_current_attribute();
        m_state = TokenizerState::BeforeAttributeName;
    } else if (current_char() == '>') {
        m_token_builder.finish_current_attribute();
        advance();
        m_state = TokenizerState::Data;
        return create_start_tag_token();
    } else if (current_char() == '\0') {
        handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in attribute value");
        return {};
    } else if (current_char() == '"' || current_char() == '\'' || current_char() == '=') {
        handle_parse_error(ErrorCode::InvalidToken, "Unexpected quote or equal sign in unquoted attribute value");
        m_token_builder.attr_value += current_char();
        advance();
    } else {
        m_token_builder.attr_value += current_char();
        advance();
    }
    return {};
}

std::optional<Token> Tokenizer::consume_self_closing_start_tag_state() {
    if (current_char() == '>') {
        advance();
        m_state = TokenizerState::Data;
        return create_close_self_token();
    }
    if (current_char() == '\0') {
        handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in self-closing tag");
        return {};
    }
    handle_parse_error(ErrorCode::InvalidToken, "Unexpected character after '/' in self-closing start tag");
    m_state = TokenizerState::BeforeAttributeName;
    return {};
}

std::optional<Token> Tokenizer::consume_comment_state() {
    if (starts_with("--")) {
        advance();
        advance();
    }
    m_char_ref_buffer.clear();
    while (has_more()) {
        if (current_char() == '-' && peek_char() == '-') {
            if (peek_char(2) == '>') {
                m_pos += 3;
                m_state                    = TokenizerState::Data;
                const auto comment_content = m_char_ref_buffer;
                m_char_ref_buffer.clear();
                return create_comment_token(comment_content);
            }
            m_char_ref_buffer += current_char();
            advance();
        } else {
            m_char_ref_buffer += current_char();
            advance();
        }
    }
    handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in comment");
    m_state                    = TokenizerState::Data;
    const auto comment_content = m_char_ref_buffer;
    m_char_ref_buffer.clear();
    return create_comment_token(comment_content);
}

std::optional<Token> Tokenizer::consume_doctype_state() {
    if (starts_with("DOCTYPE") || starts_with("doctype")) {
        m_pos += 7;
    } else {
        handle_parse_error(ErrorCode::InvalidToken, "Expected DOCTYPE keyword");
        while (has_more() && current_char() != '>') {
            advance();
        }
        if (current_char() == '>') {
            advance();
        }
        m_state = TokenizerState::Data;
        return {};
    }
    skip_whitespace();
    while (has_more() && current_char() != '>') {
        advance();
    }

    if (current_char() == '>') {
        advance();
        m_state = TokenizerState::Data;
        m_token_builder.reset();
        m_token_builder.tag_name = "DOCTYPE";
        return create_doctype_token();
    }

    handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in DOCTYPE");
    m_state = TokenizerState::Data;
    return {};
}

std::optional<Token> Tokenizer::consume_script_data_state() {
    const size_t start = m_pos;
    while (has_more()) {
        if (current_char() == '<' && starts_with("</script")) {
            const size_t saved_pos = m_pos;
            m_pos += 8;
            skip_whitespace();
            if (current_char() == '>') {
                if (start < saved_pos) {
                    m_pos                           = saved_pos;
                    std::string_view script_content = m_source.substr(start, saved_pos - start);
                    return create_text_token(script_content);
                }
                advance();
                m_state   = TokenizerState::Data;
                m_end_tag = "script";
                return create_end_tag_token();
            }
            m_pos = saved_pos;
            advance();
        } else {
            advance();
        }
    }
    if (start < m_pos) {
        const std::string_view script_content = m_source.substr(start, m_pos - start);
        m_state                               = TokenizerState::Data;
        return create_text_token(script_content);
    }
    handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in script data");
    m_state = TokenizerState::Data;
    return {};
}

std::optional<Token> Tokenizer::consume_rawtext_state() {
    const size_t start = m_pos;

    while (has_more()) {
        if (current_char() == '<' && (starts_with("</style") || starts_with("</noscript"))) {
            const size_t saved_pos = m_pos;
            const bool   is_style  = starts_with("</style");
            m_pos += (is_style ? 7 : 10);

            skip_whitespace();
            if (current_char() == '>') {
                if (start < saved_pos) {
                    m_pos                          = saved_pos;
                    const std::string_view content = m_source.substr(start, saved_pos - start);
                    return create_text_token(content);
                }
                advance();
                m_state = TokenizerState::Data;
                return {};
            }
            m_pos = saved_pos;
            advance();

        } else {
            advance();
        }
    }

    if (start < m_pos) {
        const std::string_view content = m_source.substr(start, m_pos - start);
        m_state                        = TokenizerState::Data;
        return create_text_token(content);
    }

    handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in RAWTEXT");
    m_state = TokenizerState::Data;
    return {};
}

std::optional<Token> Tokenizer::consume_rcdata_state() {
    const size_t start = m_pos;

    while (has_more()) {
        if (current_char() == '<' && (starts_with("</textarea") || starts_with("</title"))) {
            const size_t saved_pos   = m_pos;
            const bool   is_textarea = starts_with("</textarea");
            m_pos += (is_textarea ? 10 : 7);

            skip_whitespace();
            if (current_char() == '>') {
                if (start < saved_pos) {
                    m_pos                    = saved_pos;
                    std::string_view content = m_source.substr(start, saved_pos - start);
                    return create_text_token(content);
                }
                advance();
                m_state = TokenizerState::Data;
                return {};
            }
            m_pos = saved_pos;
            advance();

        } else {
            advance();
        }
    }

    if (start < m_pos) {
        const std::string_view content = m_source.substr(start, m_pos - start);
        m_state                        = TokenizerState::Data;
        return create_text_token(content);
    }

    handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in RCDATA");
    m_state = TokenizerState::Data;
    return {};
}

char Tokenizer::current_char() const noexcept {
    if (m_pos >= m_source.length()) {
        return '\0';
    }
    return m_source[m_pos];
}

char Tokenizer::peek_char(const size_t offset) const noexcept {
    const size_t peek_pos = m_pos + offset;
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

Token Tokenizer::create_start_tag_token() {
    Token token(TokenType::OPEN, m_token_builder.tag_name, "");
    for (const auto& attr : m_token_builder.attrs) {
        token.add_attr(attr);
    }

    if (m_options.is_void_element(m_token_builder.tag_name)) {
        token.set_type(TokenType::CLOSE_SELF);
    }

    if (m_token_builder.tag_name == "script") {
        m_state = TokenizerState::ScriptData;
    }

    m_token_builder.reset();
    return token;
}

Token Tokenizer::create_end_tag_token() {
    Token token(TokenType::CLOSE, m_end_tag, "");
    m_end_tag.clear();
    return token;
}

Token Tokenizer::create_text_token(std::string_view data) {
    return {TokenType::TEXT, "", data};
}

Token Tokenizer::create_comment_token(std::string_view comment) {
    return {TokenType::COMMENT, "", comment};
}

Token Tokenizer::create_doctype_token() {
    Token token(TokenType::DOCTYPE, m_token_builder.tag_name, "");
    m_token_builder.reset();
    return token;
}

Token Tokenizer::create_close_self_token() {
    Token token(TokenType::CLOSE_SELF, m_token_builder.tag_name, "");
    for (const auto& attr : m_token_builder.attrs) {
        token.add_attr(attr);
    }
    m_token_builder.reset();
    return token;
}

Token Tokenizer::create_done_token() {
    return {TokenType::DONE, "", ""};
}

void Tokenizer::handle_parse_error(const ErrorCode code, const std::string& message) {
    record_error(code, message);

    switch (m_options.error_handling) {
        case ErrorHandlingMode::Strict:
            throw HPSException(code, message, m_pos);
        case ErrorHandlingMode::Lenient:
            transition_to_data_state();
            break;
        case ErrorHandlingMode::Ignore:
            break;
    }
}

void Tokenizer::record_error(ErrorCode code, const std::string& message) {
    m_errors.emplace_back(code, message, m_pos);
}

void Tokenizer::transition_to_data_state() {
    m_state = TokenizerState::Data;
    m_token_builder.reset();
}

}  // namespace hps