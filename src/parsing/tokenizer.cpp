#include "hps/parsing/tokenizer.hpp"

#include "hps/utils/exception.hpp"
#include "hps/utils/string_utils.hpp"

#include <cstring>

namespace {

[[nodiscard]] auto text_parsing_state_for_tag(const std::string_view tag_name) noexcept -> hps::TokenizerState {
    if (hps::equals_ignore_case(tag_name, "script")) {
        return hps::TokenizerState::ScriptData;
    }
    // 注意：<svg>/<math> 不是 RAWTEXT——其内容是外来标记，必须按标签词法化，
    // 由 tree builder 以对应命名空间构建子树。
    if (hps::equals_ignore_case(tag_name, "style") || hps::equals_ignore_case(tag_name, "noscript")) {
        return hps::TokenizerState::RAWTEXT;
    }
    if (hps::equals_ignore_case(tag_name, "textarea") || hps::equals_ignore_case(tag_name, "title")) {
        return hps::TokenizerState::RCDATA;
    }
    if (hps::equals_ignore_case(tag_name, "plaintext")) {
        return hps::TokenizerState::Plaintext;
    }
    return hps::TokenizerState::Data;
}

}  // namespace

namespace hps {

Tokenizer::Tokenizer(const std::string_view source, const Options& options)
    : Tokenizer(source, options, TokenizerState::Data, {}) {}

Tokenizer::Tokenizer(
    const std::string_view source,
    const Options&         options,
    const TokenizerState   initial_state,
    const std::string_view last_start_tag)
    : m_source(source),
      m_pos(0),
      m_state(initial_state),
      m_options(options),
      m_last_start_tag(last_start_tag),
      m_attr_value_start(0) {}

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
            case TokenizerState::Plaintext:
                if (auto token = consume_plaintext_state())
                    return token;
                break;
            case TokenizerState::CDataSection:
                if (auto token = consume_cdata_section_state())
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

std::vector<HPSError> Tokenizer::consume_errors() {
    return std::move(m_errors);
}

std::optional<Token> Tokenizer::consume_data_state() {
    if (current_char() == '<') {
        advance();
        m_state = TokenizerState::TagOpen;
        return {};
    }

    const size_t start = m_pos;
    // 用 memchr 批量跳到下一个 '<'（CRT 的 SIMD 实现），代替逐字符扫描。
    // advance() 仅 ++m_pos 无副作用，故直接置位等价。'&' 不在此停（实体在文本 token
    // 整体解码），因此只需找 '<'。
    const char* const base = m_source.data();
    const void* const hit  = std::memchr(base + m_pos, '<', m_source.size() - m_pos);
    m_pos                  = hit != nullptr ? static_cast<size_t>(static_cast<const char*>(hit) - base) : m_source.size();
    if (start < m_pos) {
        return emit_text_token(m_source.substr(start, m_pos - start));
    }
    return {};
}

std::optional<Token> Tokenizer::consume_tag_open_state() {
    if (current_char() == '!') {
        advance();
        if (starts_with_ci("DOCTYPE")) {
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
            return emit_owned_text_token(std::move(cdata_content));
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
        return create_text_token(std::string_view("<"));
    }
    return {};
}

std::optional<Token> Tokenizer::consume_tag_name_state() {
    const size_t start = m_pos;
    while (m_pos < m_source.length() && is_alnum(m_source[m_pos])) {
        m_pos++;
    }

    if (m_pos > start) {
        const std::string_view raw_name = m_source.substr(start, m_pos - start);
        if (m_options.preserve_case) {
            m_token_builder.tag_name += raw_name;
        } else {
            m_token_builder.tag_name.reserve(m_token_builder.tag_name.size() + raw_name.size());
            for (const char c : raw_name) {
                m_token_builder.tag_name += to_lower(c);
            }
        }
    }

    if (!has_more()) {
        handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in tag name");
        return {};
    }

    const char c = current_char();
    if (is_whitespace(c)) {
        skip_whitespace();
        m_state = TokenizerState::BeforeAttributeName;
    } else if (c == '>') {
        advance();
        m_state = TokenizerState::Data;
        return create_start_tag_token();
    } else if (c == '/') {
        advance();
        m_state = TokenizerState::SelfClosingStartTag;
    } else {
        // HTML5 “tag name state”的 “anything else”：把当前字符并入标签名并前进。
        // 否则遇到 '<'（如 <div<div>）、'-'（自定义元素 <my-el>）等非字母数字字符，
        // 会在标签名状态原地不前进 → 死循环（DoS）。
        m_token_builder.tag_name += m_options.preserve_case ? c : to_lower(c);
        advance();
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
    if (!has_more()) {
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

    if (!has_more()) {
        handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF before attribute name");
        return {};
    }

    if (current_char() == '\'' || current_char() == '"' || current_char() == '=') {
        handle_parse_error(ErrorCode::InvalidToken, "Unexpected character before attribute name");
        advance();
        return {};
    }
    m_token_builder.attr_name.clear();
    m_state = TokenizerState::AttributeName;
    return {};
}

std::optional<Token> Tokenizer::consume_attribute_name_state() {
    const size_t start = m_pos;
    while (m_pos < m_source.length()) {
        const char c = m_source[m_pos];
        if (is_alnum(c) || c == '-' || c == '_' || c == ':') {
            m_pos++;
        } else {
            break;
        }
    }

    if (m_pos > start) {
        const std::string_view raw_name = m_source.substr(start, m_pos - start);
        if (m_options.preserve_case) {
            m_token_builder.attr_name += raw_name;
        } else {
            m_token_builder.attr_name.reserve(m_token_builder.attr_name.size() + raw_name.size());
            for (const char c : raw_name) {
                m_token_builder.attr_name += to_lower(c);
            }
        }
    }

    if (!has_more()) {
        handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in attribute name");
        return {};
    }

    const char c = current_char();
    if (is_whitespace(c)) {
        skip_whitespace();
        m_state = TokenizerState::AfterAttributeName;
    } else if (c == '=') {
        advance();
        m_state = TokenizerState::BeforeAttributeValue;
    } else if (c == '>') {
        finish_boolean_attribute();
        advance();
        m_state = TokenizerState::Data;
        return create_start_tag_token();
    } else if (c == '/') {
        finish_boolean_attribute();
        advance();
        m_state = TokenizerState::SelfClosingStartTag;
    } else {
        // HTML5 “attribute name state”的 “anything else”：并入属性名并前进。
        // '"' / '\'' / '<' 为 unexpected-character-in-attribute-name，'\0' 为
        // unexpected-null-character —— 均记为可恢复错误但仍并入。绝不回退到
        // before-attribute-name，否则会与该状态来回死循环（DoS，html5lib 套件触发）。
        if (c == '"' || c == '\'' || c == '<' || c == '\0') {
            record_recoverable_error(ErrorCode::InvalidToken, "Unexpected character in attribute name");
        }
        m_token_builder.attr_name += m_options.preserve_case ? c : to_lower(c);
        advance();
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
        finish_boolean_attribute();
        advance();
        m_state = TokenizerState::Data;
        return create_start_tag_token();
    } else if (current_char() == '/') {
        finish_boolean_attribute();
        advance();
        m_state = TokenizerState::SelfClosingStartTag;
    } else if (!has_more()) {
        handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF after attribute name");
        return {};
    } else {
        finish_boolean_attribute();
        m_state = TokenizerState::AttributeName;
    }
    return {};
}

std::optional<Token> Tokenizer::consume_before_attribute_value_state() {
    if (is_whitespace(current_char())) {
        skip_whitespace();
    } else if (current_char() == '"') {
        advance();
        m_attr_value_start = m_pos;
        m_state            = TokenizerState::AttributeValueDoubleQuoted;
    } else if (current_char() == '\'') {
        advance();
        m_attr_value_start = m_pos;
        m_state            = TokenizerState::AttributeValueSingleQuoted;
    } else if (current_char() == '>') {
        record_error(ErrorCode::InvalidToken, "Missing attribute value");
        if (m_options.error_handling == ErrorHandlingMode::Strict) {
            throw HPSException(ErrorCode::InvalidToken, "Missing attribute value", Location::from_position(m_source, m_pos));
        }
        finish_boolean_attribute();
        advance();
        m_state = TokenizerState::Data;
        return create_start_tag_token();
    } else {
        m_attr_value_start = m_pos;
        advance();
        m_state = TokenizerState::AttributeValueUnquoted;
    }
    return {};
}

std::optional<Token> Tokenizer::consume_attribute_value_double_quoted_state() {
    const size_t start = m_attr_value_start;
    while (has_more()) {
        if (current_char() == '"') {
            const std::string_view value = m_source.substr(start, m_pos - start);
            finish_attribute(value);
            advance();
            if (current_char() != '\0' && !is_whitespace(current_char()) && current_char() != '>' && current_char() != '/') {
                record_recoverable_error(ErrorCode::InvalidToken, "Missing whitespace between attributes");
            }
            m_state = TokenizerState::BeforeAttributeName;
            return {};
        }
        advance();
    }
    handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in attribute value");
    return {};
}

std::optional<Token> Tokenizer::consume_attribute_value_single_quoted_state() {
    const size_t start = m_attr_value_start;
    while (has_more()) {
        if (current_char() == '\'') {
            const std::string_view value = m_source.substr(start, m_pos - start);
            finish_attribute(value);
            advance();
            if (current_char() != '\0' && !is_whitespace(current_char()) && current_char() != '>' && current_char() != '/') {
                record_recoverable_error(ErrorCode::InvalidToken, "Missing whitespace between attributes");
            }
            m_state = TokenizerState::BeforeAttributeName;
            return {};
        }
        advance();
    }
    handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in attribute value");
    return {};
}

std::optional<Token> Tokenizer::consume_attribute_value_unquoted_state() {
    const size_t start = m_attr_value_start;
    while (has_more()) {
        const char c = current_char();
        if (is_whitespace(c)) {
            const std::string_view value = m_source.substr(start, m_pos - start);
            finish_attribute(value);
            m_state = TokenizerState::BeforeAttributeName;
            return {};
        } else if (c == '>') {
            const std::string_view value = m_source.substr(start, m_pos - start);
            finish_attribute(value);
            advance();
            m_state = TokenizerState::Data;
            return create_start_tag_token();
        } else if (c == '"' || c == '\'' || c == '=' || c == '`' || c == '<') {
            handle_parse_error(ErrorCode::InvalidToken, "Unexpected character in unquoted attribute value");
        }
        advance();
    }
    handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in attribute value");
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
    record_error(ErrorCode::InvalidToken, "Unexpected character after '/' in self-closing start tag");
    if (m_options.error_handling == ErrorHandlingMode::Strict) {
        throw HPSException(
            ErrorCode::InvalidToken,
            "Unexpected character after '/' in self-closing start tag",
            Location::from_position(m_source, m_pos));
    }
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
                m_state              = TokenizerState::Data;
                auto comment_content = std::move(m_char_ref_buffer);
                m_char_ref_buffer.clear();
                return create_owned_comment_token(std::move(comment_content));
            }
            m_char_ref_buffer += current_char();
            advance();
        } else {
            m_char_ref_buffer += current_char();
            advance();
        }
    }
    handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in comment");
    m_state              = TokenizerState::Data;
    auto comment_content = std::move(m_char_ref_buffer);
    m_char_ref_buffer.clear();
    return create_owned_comment_token(std::move(comment_content));
}

std::optional<Token> Tokenizer::consume_doctype_state() {
    if (starts_with_ci("DOCTYPE")) {
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

    m_token_builder.reset();
    skip_whitespace();

    while (has_more()) {
        const char c = current_char();
        if (c == '>' || is_whitespace(c)) {
            break;
        }

        if (is_alpha(c)) {
            m_token_builder.tag_name += to_lower(c);
        } else {
            m_token_builder.tag_name += c;
        }
        advance();
    }

    skip_whitespace();

    // 解析可选的 PUBLIC / SYSTEM 标识符（HTML5 DOCTYPE 标识符各状态的精简实现）。
    const auto match_keyword_ci = [this](const std::string_view keyword) -> bool {
        if (m_pos + keyword.size() > m_source.length()) {
            return false;
        }
        for (size_t i = 0; i < keyword.size(); ++i) {
            if (to_lower(m_source[m_pos + i]) != to_lower(keyword[i])) {
                return false;
            }
        }
        m_pos += keyword.size();
        return true;
    };
    const auto read_quoted_id = [this](std::string& out) {
        const char quote = current_char();
        if (quote != '"' && quote != '\'') {
            return;
        }
        advance();
        while (has_more() && current_char() != quote && current_char() != '>') {
            out += current_char();
            advance();
        }
        if (has_more() && current_char() == quote) {
            advance();
        }
    };

    if (match_keyword_ci("PUBLIC")) {
        m_token_builder.doctype_has_identifiers = true;
        skip_whitespace();
        read_quoted_id(m_token_builder.doctype_public_id);
        skip_whitespace();
        read_quoted_id(m_token_builder.doctype_system_id);
    } else if (match_keyword_ci("SYSTEM")) {
        m_token_builder.doctype_has_identifiers = true;
        skip_whitespace();
        read_quoted_id(m_token_builder.doctype_system_id);
    }

    while (has_more() && current_char() != '>') {
        advance();
    }

    if (current_char() == '>') {
        advance();
        m_state = TokenizerState::Data;
        return create_doctype_token();
    }

    m_token_builder.force_quirks = true;
    record_recoverable_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in DOCTYPE");
    m_state = TokenizerState::Data;
    return create_doctype_token();
}

std::optional<Token> Tokenizer::consume_script_data_state() {
    const std::string closing = m_last_start_tag.empty() ? std::string("script") : std::string(m_last_start_tag);
    const size_t      start   = m_pos;

    // HTML5 script-data 转义层级：0=普通，1=escaped(<!-- 后)，2=double-escaped(<!-- … <script 后)。
    // 仅在层级 0/1 时 </script> 才闭合脚本；层级 2 时 </script> 只退回层级 1。
    int escape = 0;

    while (has_more()) {
        // </script + 终止符（空白 / '/' / '>' / EOF）。
        if (current_char() == '<' && peek_char() == '/' &&
            starts_with_ignore_case(m_source.substr(m_pos + 2), closing)) {
            const size_t saved_pos = m_pos;
            const size_t after     = m_pos + 2 + closing.size();
            const bool   appropriate =
                after >= m_source.size() || is_whitespace(m_source[after]) ||
                m_source[after] == '/' || m_source[after] == '>';

            if (appropriate && escape == 2) {
                escape = 1;  // double-escaped：</script> 不闭合，退回 escaped。
                m_pos  = after;
                continue;
            }
            if (appropriate) {
                if (start < saved_pos) {
                    // 先发出已收集的脚本内容；下次调用从 </script 处再出结束标签。
                    m_pos = saved_pos;
                    return emit_text_token(m_source.substr(start, saved_pos - start));
                }
                // 消费结束标签的属性/空白直到 '>'（引号内的 '>' 不算闭合）。
                m_pos = after;
                while (has_more() && current_char() != '>') {
                    if (const char ch = current_char(); ch == '"' || ch == '\'') {
                        advance();
                        while (has_more() && current_char() != ch) {
                            advance();
                        }
                    }
                    advance();
                }
                if (has_more() && current_char() == '>') {
                    advance();
                }
                m_state   = TokenizerState::Data;
                m_end_tag = closing;
                return create_end_tag_token();
            }
            m_pos = saved_pos;  // 非合适结束标签（如 </scriptx）：当作内容。
            advance();
            continue;
        }

        // 进入 escaped：<!--（仅普通态）。
        if (escape == 0 && current_char() == '<' && m_source.compare(m_pos, 4, "<!--") == 0) {
            escape = 1;
            m_pos += 4;
            continue;
        }
        // escaped → double-escaped：<script + 终止符。
        if (escape == 1 && current_char() == '<' &&
            starts_with_ignore_case(m_source.substr(m_pos + 1), "script")) {
            if (const size_t after = m_pos + 1 + 6;
                after >= m_source.size() || is_whitespace(m_source[after]) ||
                m_source[after] == '/' || m_source[after] == '>') {
                escape = 2;
                m_pos  = after;
                continue;
            }
        }
        // escaped / double-escaped → 普通：-->。
        if (escape >= 1 && current_char() == '-' && m_source.compare(m_pos, 3, "-->") == 0) {
            escape = 0;
            m_pos += 3;
            continue;
        }

        advance();
    }

    if (start < m_pos) {
        m_state = TokenizerState::Data;
        return emit_text_token(m_source.substr(start, m_pos - start));
    }
    handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in script data");
    m_state = TokenizerState::Data;
    return {};
}

std::optional<Token> Tokenizer::consume_rawtext_state() {
    return consume_raw_text_until_end_tag(/*decode_entities=*/false);
}

std::optional<Token> Tokenizer::consume_rcdata_state() {
    return consume_raw_text_until_end_tag(/*decode_entities=*/true);
}

std::optional<Token> Tokenizer::consume_raw_text_until_end_tag(const bool decode_entities) {
    if (m_last_start_tag.empty()) {
        m_state = TokenizerState::Data;
        return {};
    }

    const std::string_view closing_tag = m_last_start_tag;
    const size_t           start       = m_pos;

    // RAWTEXT 原样输出；RCDATA 解码字符实体（这是两态唯一的差别）。
    const auto emit_collected = [this, decode_entities](const std::string_view content) -> std::optional<Token> {
        if (decode_entities) {
            return emit_owned_text_token(decode_html_entities(std::string(content)));
        }
        return emit_text_token(content);
    };

    while (has_more()) {
        if (current_char() == '<' && peek_char() == '/' &&
            starts_with_ignore_case(m_source.substr(m_pos + 2), closing_tag)) {
            const size_t saved_pos = m_pos;
            m_pos += 2 + closing_tag.size();

            if (!has_more()) {
                m_pos = m_source.size();
                break;
            }

            if (is_whitespace(current_char())) {
                while (has_more() && is_whitespace(current_char())) {
                    advance();
                }
                if (!has_more()) {
                    record_recoverable_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in raw text end tag");
                    m_state = TokenizerState::Data;
                    if (start < saved_pos) {
                        return emit_collected(m_source.substr(start, saved_pos - start));
                    }
                    return {};
                }
            }
            if (current_char() == '>') {
                if (start < saved_pos) {
                    m_pos = saved_pos;
                    return emit_collected(m_source.substr(start, saved_pos - start));
                }
                advance();
                m_state   = TokenizerState::Data;
                m_end_tag = std::string(closing_tag);
                return create_end_tag_token();
            }
            if (current_char() == '/') {
                advance();
                while (has_more() && is_whitespace(current_char())) {
                    advance();
                }
                if (!has_more()) {
                    record_recoverable_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in raw text end tag");
                    m_state = TokenizerState::Data;
                    if (start < saved_pos) {
                        return emit_collected(m_source.substr(start, saved_pos - start));
                    }
                    return {};
                }
                if (current_char() == '>') {
                    if (start < saved_pos) {
                        m_pos = saved_pos;
                        return emit_collected(m_source.substr(start, saved_pos - start));
                    }
                    advance();
                    m_state   = TokenizerState::Data;
                    m_end_tag = std::string(closing_tag);
                    return create_end_tag_token();
                }
            }
            m_pos = saved_pos;
            advance();
        } else {
            // 非结束标签处：用 memchr 跳到下一个 '<'（至少前进一个字符，避免停在当前 '<'）。
            const size_t from = m_pos + 1;
            if (from >= m_source.size()) {
                m_pos = m_source.size();
            } else {
                const char* const b = m_source.data();
                const void* const h = std::memchr(b + from, '<', m_source.size() - from);
                m_pos = h != nullptr ? static_cast<size_t>(static_cast<const char*>(h) - b) : m_source.size();
            }
        }
    }

    if (start < m_pos) {
        m_state = TokenizerState::Data;
        return emit_collected(m_source.substr(start, m_pos - start));
    }

    handle_parse_error(ErrorCode::UnexpectedEOF, "Unexpected EOF in raw text");
    m_state = TokenizerState::Data;
    return {};
}

std::optional<Token> Tokenizer::consume_plaintext_state() {
    const size_t start = m_pos;
    m_pos              = m_source.size();  // plaintext 一直消费到结尾
    if (start < m_pos) {
        return emit_text_token(m_source.substr(start, m_pos - start));
    }
    return {};
}

std::optional<Token> Tokenizer::consume_cdata_section_state() {
    std::string cdata_content;
    while (has_more()) {
        if (starts_with("]]>")) {
            m_pos += 3;
            break;
        }
        cdata_content += current_char();
        advance();
    }
    m_state = TokenizerState::Data;
    return emit_owned_text_token(std::move(cdata_content));
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

bool Tokenizer::starts_with_ci(std::string_view s) const noexcept {
    if (m_pos + s.length() > m_source.length()) {
        return false;
    }
    for (size_t i = 0; i < s.length(); ++i) {
        if (to_lower(m_source[m_pos + i]) != to_lower(s[i])) {
            return false;
        }
    }
    return true;
}

Token Tokenizer::create_start_tag_token() {
    Token token(TokenType::OPEN, m_token_builder.tag_name, "");
    for (auto& attr : m_token_builder.attrs) {
        token.add_attr(std::move(attr));
    }

    if (m_options.is_void_element(m_token_builder.tag_name)) {
        token.set_type(TokenType::CLOSE_SELF);
    }

    const auto next_state = text_parsing_state_for_tag(m_token_builder.tag_name);
    m_last_start_tag      = m_token_builder.tag_name;
    if (token.type() == TokenType::OPEN && next_state != TokenizerState::Data) {
        m_state = next_state;
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

Token Tokenizer::create_owned_text_token(std::string&& data) {
    Token token(TokenType::TEXT, "", "");
    token.set_owned_value(std::move(data));
    return token;
}

std::optional<Token> Tokenizer::emit_text_token(std::string_view data) {
    if (data.empty()) {
        return {};
    }

    if (data.size() > m_options.max_text_length) {
        record_error(ErrorCode::TextTooLong, "Text node length limit exceeded");
        if (m_options.error_handling == ErrorHandlingMode::Strict) {
            throw HPSException(
                ErrorCode::TextTooLong,
                "Text node length limit exceeded",
                Location::from_position(m_source, m_pos));
        }
        data = data.substr(0, m_options.max_text_length);
    }

    return create_text_token(data);
}

std::optional<Token> Tokenizer::emit_owned_text_token(std::string data) {
    if (data.empty()) {
        return {};
    }

    if (data.size() > m_options.max_text_length) {
        record_error(ErrorCode::TextTooLong, "Text node length limit exceeded");
        if (m_options.error_handling == ErrorHandlingMode::Strict) {
            throw HPSException(
                ErrorCode::TextTooLong,
                "Text node length limit exceeded",
                Location::from_position(m_source, m_pos));
        }
        data.resize(m_options.max_text_length);
    }

    return create_owned_text_token(std::move(data));
}

Token Tokenizer::create_comment_token(std::string_view comment) {
    return {TokenType::COMMENT, "", comment};
}

Token Tokenizer::create_owned_comment_token(std::string&& comment) {
    Token token(TokenType::COMMENT, "", "");
    token.set_owned_value(std::move(comment));
    return token;
}

Token Tokenizer::create_doctype_token() {
    Token token(TokenType::DOCTYPE, m_token_builder.tag_name, "");
    token.set_doctype_identifiers(
        m_token_builder.doctype_public_id,
        m_token_builder.doctype_system_id,
        m_token_builder.doctype_has_identifiers);
    token.set_doctype_force_quirks(m_token_builder.force_quirks);
    m_token_builder.reset();
    return token;
}

Token Tokenizer::create_close_self_token() {
    Token token(TokenType::CLOSE_SELF, m_token_builder.tag_name, "");
    for (auto& attr : m_token_builder.attrs) {
        token.add_attr(std::move(attr));
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
            throw HPSException(code, message, Location::from_position(m_source, m_pos));
        case ErrorHandlingMode::Lenient:
            transition_to_data_state();
            break;
        case ErrorHandlingMode::Ignore:
            break;
    }
}

void Tokenizer::record_recoverable_error(const ErrorCode code, const std::string& message) {
    record_error(code, message);
    if (m_options.error_handling == ErrorHandlingMode::Strict) {
        throw HPSException(code, message, Location::from_position(m_source, m_pos));
    }
}

Location Tokenizer::location_at_pos() noexcept {
    const size_t end = m_pos > m_source.size() ? m_source.size() : m_pos;
    if (end < m_loc_pos) {
        // 罕见的位置回退：全量计算，且不污染增量游标。
        return Location::from_position(m_source, end);
    }
    for (size_t i = m_loc_pos; i < end; ++i) {
        if (m_source[i] == '\n') {
            ++m_loc_line;
            m_loc_col = 1;
        } else {
            ++m_loc_col;
        }
    }
    m_loc_pos = end;
    return Location(end, m_loc_line, m_loc_col);
}

void Tokenizer::record_error(ErrorCode code, const std::string& message) {
    // 增量行列计算：避免每个错误都从头扫描源码（大文档上会退化为 O(n²)）。
    m_errors.emplace_back(code, message, location_at_pos());
}

void Tokenizer::transition_to_data_state() {
    m_state = TokenizerState::Data;
    m_token_builder.reset();
}

void Tokenizer::finish_boolean_attribute() {
    if (m_token_builder.attr_name.empty()) {
        return;
    }

    if (m_token_builder.attrs.size() >= m_options.max_attributes) {
        record_error(ErrorCode::TooManyAttributes, "Attribute count limit exceeded");
        if (m_options.error_handling == ErrorHandlingMode::Strict) {
            throw HPSException(
                ErrorCode::TooManyAttributes,
                "Attribute count limit exceeded",
                Location::from_position(m_source, m_pos));
        }
        m_token_builder.attr_name.clear();
        return;
    }

    if (m_token_builder.attr_name.size() > m_options.max_attribute_name_length) {
        record_error(ErrorCode::AttributeTooLong, "Attribute name length limit exceeded");
        if (m_options.error_handling == ErrorHandlingMode::Strict) {
            throw HPSException(
                ErrorCode::AttributeTooLong,
                "Attribute name length limit exceeded",
                Location::from_position(m_source, m_pos));
        }
        m_token_builder.attr_name.clear();
        return;
    }

    m_token_builder.add_attr(std::move(m_token_builder.attr_name), "", false);
    m_token_builder.attr_name.clear();
}

void Tokenizer::finish_attribute(const std::string_view value) {
    if (m_token_builder.attr_name.empty()) {
        return;
    }

    if (m_token_builder.attrs.size() >= m_options.max_attributes) {
        record_error(ErrorCode::TooManyAttributes, "Attribute count limit exceeded");
        if (m_options.error_handling == ErrorHandlingMode::Strict) {
            throw HPSException(
                ErrorCode::TooManyAttributes,
                "Attribute count limit exceeded",
                Location::from_position(m_source, m_pos));
        }
        m_token_builder.attr_name.clear();
        return;
    }

    if (m_token_builder.attr_name.size() > m_options.max_attribute_name_length) {
        record_error(ErrorCode::AttributeTooLong, "Attribute name length limit exceeded");
        if (m_options.error_handling == ErrorHandlingMode::Strict) {
            throw HPSException(
                ErrorCode::AttributeTooLong,
                "Attribute name length limit exceeded",
                Location::from_position(m_source, m_pos));
        }
        m_token_builder.attr_name.clear();
        return;
    }

    auto stored_value = value;
    if (stored_value.size() > m_options.max_attribute_value_length) {
        record_error(ErrorCode::AttributeTooLong, "Attribute value length limit exceeded");
        if (m_options.error_handling == ErrorHandlingMode::Strict) {
            throw HPSException(
                ErrorCode::AttributeTooLong,
                "Attribute value length limit exceeded",
                Location::from_position(m_source, m_pos));
        }
        stored_value = stored_value.substr(0, m_options.max_attribute_value_length);
    }

    m_token_builder.add_attr(std::move(m_token_builder.attr_name), stored_value, true);
    m_token_builder.attr_name.clear();
}

}  // namespace hps
