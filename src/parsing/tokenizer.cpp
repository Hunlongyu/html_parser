#include "hps/parsing/tokenizer.hpp"

#include "hps/utils/exception.hpp"

#include <unordered_map>
#include <unordered_set>

namespace hps {

static const std::unordered_set<std::string_view> VOID_ELEMENTS = {"area",
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

static const std::unordered_map<std::string, char> NAMED_CHARACTER_REFERENCES = {
    {"lt", '<'},
    {"gt", '>'},
    {"amp", '&'},
    {"apos", '\''},
    {"quot", '"'},
    {"nbsp", '\xA0'},
    {"copy", '\xA9'},
    {"reg", '\xAE'},
};

Tokenizer::Tokenizer(std::string_view source, ErrorHandlingMode mode)
    : m_source(source),
      m_pos(0),
      m_state(TokenizerState::Data),
      m_error_mode(mode),
      m_return_state(TokenizerState::Data) {}

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
            case TokenizerState::CommentStart:
                if (auto token = consume_comment_start_state())
                    return token;
                break;
            case TokenizerState::Comment:
                if (auto token = consume_comment_state())
                    return token;
                break;
            case TokenizerState::CommentEndDash:
                if (auto token = consume_comment_end_dash_state())
                    return token;
                break;
            case TokenizerState::CommentEnd:
                if (auto token = consume_comment_end_state())
                    return token;
                break;
            case TokenizerState::DOCTYPE:
                if (auto token = consume_doctype_state())
                    return token;
                break;
            case TokenizerState::DOCTYPEName:
                if (auto token = consume_doctype_name_state())
                    return token;
                break;
            case TokenizerState::AfterDOCTYPEName:
                if (auto token = consume_after_doctype_name_state())
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
            case TokenizerState::CharacterReference:
                if (auto token = consume_character_reference_state())
                    return token;
                break;
            case TokenizerState::NamedCharacterReference:
                if (auto token = consume_named_character_reference_state())
                    return token;
                break;
            case TokenizerState::NumericCharacterReference:
                if (auto token = consume_numeric_character_reference_state())
                    return token;
                break;
            case TokenizerState::MarkupDeclarationOpen:
                if (auto token = consume_markup_declaration_open_state())
                    return token;
                break;
            case TokenizerState::CDATASection:
                if (auto token = consume_cdata_section_state())
                    return token;
                break;
            default:
                return create_done_token();
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

const std::vector<ParseError>& Tokenizer::get_errors() const noexcept {
    return m_errors;
}

void Tokenizer::set_error_handling_mode(ErrorHandlingMode mode) noexcept {
    m_error_mode = mode;
}

std::optional<Token> Tokenizer::consume_data_state() {
    if (current_char() == '<') {
        advance();
        m_state = TokenizerState::TagOpen;
        return {};
    }

    if (current_char() == '&') {
        m_return_state = TokenizerState::Data;
        m_state        = TokenizerState::CharacterReference;
        return {};
    }

    size_t start = m_pos;
    while (has_more() && current_char() != '<' && current_char() != '&') {
        advance();
    }
    if (start < m_pos) {
        std::string_view data = m_source.substr(start, m_pos - start);
        return create_text_token(data);
    }
    return {};
}

std::optional<Token> Tokenizer::consume_tag_open_state() {
    if (current_char() == '!') {
        advance();
        m_state = TokenizerState::MarkupDeclarationOpen;
    } else if (current_char() == '/') {
        advance();
        m_state = TokenizerState::EndTagOpen;
    } else if (is_alpha(current_char())) {
        m_token_builder.reset();
        m_state = TokenizerState::TagName;
    } else if (current_char() == '?') {
        // 处理 XML 声明或处理指令
        advance();
        while (has_more() && current_char() != '>') {
            advance();
        }
        if (current_char() == '>') {
            advance();
        }
        m_state = TokenizerState::Data;
    } else {
        // 不是有效的标签开始，作为文本处理
        m_state = TokenizerState::Data;
        return create_text_token("<");
    }
    return {};
}

std::optional<Token> Tokenizer::consume_tag_name_state() {
    while (has_more() && is_alnum(current_char())) {
        m_token_builder.tag_name += to_lower(current_char());
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
        handle_parse_error(ParseException::ErrorCode::UnexpectedEOF, "Unexpected EOF in tag name");
        return {};
    }
    return {};
}

std::optional<Token> Tokenizer::consume_end_tag_open_state() {
    if (is_alpha(current_char())) {
        m_end_tag.clear();
        m_state = TokenizerState::EndTagName;
    } else if (current_char() == '>') {
        handle_parse_error(ParseException::ErrorCode::InvalidToken, "Empty end tag");
        advance();
        m_state = TokenizerState::Data;
    } else {
        handle_parse_error(ParseException::ErrorCode::InvalidToken, "Invalid character after </");
        m_state = TokenizerState::Data;
    }
    return {};
}

std::optional<Token> Tokenizer::consume_end_tag_name_state() {
    while (has_more() && is_alnum(current_char())) {
        m_end_tag += to_lower(current_char());
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
        handle_parse_error(ParseException::ErrorCode::UnexpectedEOF,
                           "Unexpected EOF in end tag name");
        return {};
    }
    handle_parse_error(ParseException::ErrorCode::InvalidToken, "Invalid character in end tag");
    // 跳过到 >
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
        handle_parse_error(ParseException::ErrorCode::UnexpectedEOF,
                           "Unexpected EOF before attribute name");
        return {};
    }

    if (current_char() == '\'' || current_char() == '"' || current_char() == '=') {
        handle_parse_error(ParseException::ErrorCode::InvalidToken,
                           "Unexpected character before attribute name");
        advance();
        return {};
    }

    // 开始新属性
    m_token_builder.attr_name.clear();
    m_token_builder.attr_value.clear();
    m_state = TokenizerState::AttributeName;
    return {};
}

std::optional<Token> Tokenizer::consume_attribute_name_state() {
    // 收集完整的属性名
    while (has_more() && is_alnum(current_char()) || current_char() == '-' ||
           current_char() == '_') {
        m_token_builder.attr_name += to_lower(current_char());
        advance();
    }

    if (is_whitespace(current_char())) {
        skip_whitespace();
        m_state = TokenizerState::AfterAttributeName;
    } else if (current_char() == '=') {
        advance();
        m_state = TokenizerState::BeforeAttributeValue;
    } else if (current_char() == '>') {
        // 没有值的属性
        m_token_builder.finish_current_attribute();
        advance();
        m_state = TokenizerState::Data;
        return create_start_tag_token();
    } else if (current_char() == '/') {
        m_token_builder.finish_current_attribute();
        advance();
        m_state = TokenizerState::SelfClosingStartTag;
    } else if (current_char() == '\0') {
        handle_parse_error(ParseException::ErrorCode::UnexpectedEOF,
                           "Unexpected EOF in attribute name");
        return {};
    } else {
        // 遇到非法字符，完成当前属性
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
        handle_parse_error(ParseException::ErrorCode::UnexpectedEOF,
                           "Unexpected EOF after attribute name");
        return {};
    } else {
        // 新的属性开始，先完成当前属性
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
        handle_parse_error(ParseException::ErrorCode::InvalidToken, "Missing attribute value");
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
    } else if (current_char() == '&') {
        m_return_state = TokenizerState::AttributeValueDoubleQuoted;
        m_state        = TokenizerState::CharacterReference;
    } else if (current_char() == '\0') {
        handle_parse_error(ParseException::ErrorCode::UnexpectedEOF,
                           "Unexpected EOF in attribute value");
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
    } else if (current_char() == '&') {
        m_return_state = TokenizerState::AttributeValueSingleQuoted;
        m_state        = TokenizerState::CharacterReference;
    } else if (current_char() == '\0') {
        handle_parse_error(ParseException::ErrorCode::UnexpectedEOF,
                           "Unexpected EOF in attribute value");
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
    } else if (current_char() == '&') {
        m_return_state = TokenizerState::AttributeValueUnquoted;
        m_state        = TokenizerState::CharacterReference;
    } else if (current_char() == '\0') {
        handle_parse_error(ParseException::ErrorCode::UnexpectedEOF,
                           "Unexpected EOF in attribute value");
        return {};
    } else if (current_char() == '"' || current_char() == '\'' || current_char() == '=') {
        handle_parse_error(ParseException::ErrorCode::InvalidToken,
                           "Unexpected quote or equal sign in unquoted attribute value");
        // 容错处理：仍然添加这个字符
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
    } else if (current_char() == '\0') {
        handle_parse_error(ParseException::ErrorCode::UnexpectedEOF,
                           "Unexpected EOF in self-closing tag");
        return {};
    }

    handle_parse_error(ParseException::ErrorCode::InvalidToken,
                       "Unexpected character after '/' in self-closing start tag");
    m_state = TokenizerState::BeforeAttributeName;

    return {};
}

std::optional<Token> Tokenizer::consume_comment_start_state() {
    if (starts_with("--")) {
        m_pos += 2;
        m_char_ref_buffer.clear();
        m_state = TokenizerState::Comment;
    } else {
        handle_parse_error(ParseException::ErrorCode::InvalidToken, "Invalid comment start");
        // 跳过到下一个 >
        while (has_more() && current_char() != '>') {
            advance();
        }
        if (current_char() == '>') {
            advance();
        }
        m_state = TokenizerState::Data;
    }
    return {};
}

std::optional<Token> Tokenizer::consume_comment_state() {
    while (has_more()) {
        if (current_char() == '-' && peek_char() == '-') {
            if (peek_char(2) == '>') {
                // 找到注释结束
                m_pos += 3;
                m_state              = TokenizerState::Data;
                auto comment_content = m_char_ref_buffer;
                m_char_ref_buffer.clear();
                return create_comment_token(comment_content);
            }
            // 处理 -- 但不是结束，添加第一个 -
            m_char_ref_buffer += current_char();
            advance();
        } else {
            m_char_ref_buffer += current_char();
            advance();
        }
    }

    // 文件结束但注释未闭合
    handle_parse_error(ParseException::ErrorCode::UnexpectedEOF, "Unexpected EOF in comment");
    m_state              = TokenizerState::Data;
    auto comment_content = m_char_ref_buffer;
    m_char_ref_buffer.clear();
    return create_comment_token(comment_content);
}

std::optional<Token> Tokenizer::consume_comment_end_dash_state() {
    return consume_comment_state();
}

std::optional<Token> Tokenizer::consume_comment_end_state() {
    return consume_comment_state();
}

std::optional<Token> Tokenizer::consume_doctype_state() {
    skip_whitespace();
    if (starts_with("DOCTYPE")) {
        m_pos += 7;
        m_token_builder.reset();
        skip_whitespace();
        m_state = TokenizerState::DOCTYPEName;
    } else {
        handle_parse_error(ParseException::ErrorCode::InvalidToken, "Invalid DOCTYPE declaration");
        // 跳过到 >
        while (has_more() && current_char() != '>') {
            advance();
        }
        if (current_char() == '>') {
            advance();
        }
        m_state = TokenizerState::Data;
    }
    return {};
}

std::optional<Token> Tokenizer::consume_doctype_name_state() {
    while (has_more() && !is_whitespace(current_char()) && current_char() != '>') {
        m_token_builder.tag_name += to_lower(current_char());
        advance();
    }

    if (is_whitespace(current_char())) {
        skip_whitespace();
        m_state = TokenizerState::AfterDOCTYPEName;
    } else if (current_char() == '>') {
        advance();
        m_state = TokenizerState::Data;
        return create_doctype_token();
    } else if (current_char() == '\0') {
        handle_parse_error(ParseException::ErrorCode::UnexpectedEOF,
                           "Unexpected EOF in DOCTYPE name");
        return {};
    }
    return {};
}

std::optional<Token> Tokenizer::consume_after_doctype_name_state() {
    if (is_whitespace(current_char())) {
        skip_whitespace();
    } else if (current_char() == '>') {
        advance();
        m_state = TokenizerState::Data;
        return create_doctype_token();
    } else if (current_char() == '\0') {
        handle_parse_error(ParseException::ErrorCode::UnexpectedEOF,
                           "Unexpected EOF after DOCTYPE name");
        return {};
    } else {
        // 跳过其他 DOCTYPE 内容（如 PUBLIC, SYSTEM 等）
        while (has_more() && current_char() != '>') {
            advance();
        }
        if (current_char() == '>') {
            advance();
        }
        m_state = TokenizerState::Data;
        return create_doctype_token();
    }
    return {};
}

std::optional<Token> Tokenizer::consume_script_data_state() {
    size_t start = m_pos;

    while (has_more()) {
        if (current_char() == '<' && starts_with("</script")) {
            // 检查是否真的是结束标签
            size_t saved_pos = m_pos;
            m_pos += 8;  // 跳过 "</script"

            skip_whitespace();
            if (current_char() == '>') {
                // 找到真正的结束标签
                if (start < saved_pos) {
                    m_pos                           = saved_pos;
                    std::string_view script_content = m_source.substr(start, saved_pos - start);
                    return create_text_token(script_content);
                } else {
                    // 创建结束标签
                    m_pos = saved_pos + 8;
                    advance();  // 跳过 >
                    m_state = TokenizerState::Data;
                    return {};
                }
            } else {
                // 不是真正的结束标签，恢复位置继续
                m_pos = saved_pos;
                advance();
            }
        } else {
            advance();
        }
    }

    // 到达文件末尾
    if (start < m_pos) {
        std::string_view script_content = m_source.substr(start, m_pos - start);
        m_state                         = TokenizerState::Data;
        return create_text_token(script_content);
    }

    handle_parse_error(ParseException::ErrorCode::UnexpectedEOF, "Unexpected EOF in script data");
    m_state = TokenizerState::Data;
    return {};
}

std::optional<Token> Tokenizer::consume_rawtext_state() {
    size_t start = m_pos;

    while (has_more()) {
        if (current_char() == '<' && (starts_with("</style") || starts_with("</noscript"))) {
            size_t saved_pos = m_pos;
            bool   is_style  = starts_with("</style");
            m_pos += (is_style ? 7 : 10);

            skip_whitespace();
            if (current_char() == '>') {
                if (start < saved_pos) {
                    m_pos                    = saved_pos;
                    std::string_view content = m_source.substr(start, saved_pos - start);
                    return create_text_token(content);
                } else {
                    advance();  // 跳过 >
                    m_state = TokenizerState::Data;
                    return {};
                }
            } else {
                m_pos = saved_pos;
                advance();
            }
        } else {
            advance();
        }
    }

    if (start < m_pos) {
        std::string_view content = m_source.substr(start, m_pos - start);
        m_state                  = TokenizerState::Data;
        return create_text_token(content);
    }

    handle_parse_error(ParseException::ErrorCode::UnexpectedEOF, "Unexpected EOF in RAWTEXT");
    m_state = TokenizerState::Data;
    return {};
}

std::optional<Token> Tokenizer::consume_rcdata_state() {
    size_t start = m_pos;

    while (has_more()) {
        if (current_char() == '<' && (starts_with("</textarea") || starts_with("</title"))) {
            size_t saved_pos   = m_pos;
            bool   is_textarea = starts_with("</textarea");
            m_pos += (is_textarea ? 10 : 7);

            skip_whitespace();
            if (current_char() == '>') {
                if (start < saved_pos) {
                    m_pos                    = saved_pos;
                    std::string_view content = m_source.substr(start, saved_pos - start);
                    return create_text_token(content);
                } else {
                    advance();  // 跳过 >
                    m_state = TokenizerState::Data;
                    return {};
                }
            } else {
                m_pos = saved_pos;
                advance();
            }
        } else if (current_char() == '&') {
            // 在 RCDATA 中可以解析字符引用
            if (start < m_pos) {
                std::string_view content = m_source.substr(start, m_pos - start);
                return create_text_token(content);
            }
            m_return_state = TokenizerState::RCDATA;
            m_state        = TokenizerState::CharacterReference;
            return {};
        } else {
            advance();
        }
    }

    if (start < m_pos) {
        std::string_view content = m_source.substr(start, m_pos - start);
        m_state                  = TokenizerState::Data;
        return create_text_token(content);
    }

    handle_parse_error(ParseException::ErrorCode::UnexpectedEOF, "Unexpected EOF in RCDATA");
    m_state = TokenizerState::Data;
    return {};
}

std::optional<Token> Tokenizer::consume_character_reference_state() {
    if (current_char() != '&') {
        // 如果不是 &，说明状态转换有问题
        m_state = m_return_state;
        return {};
    }

    advance();  // 跳过 &

    if (is_alpha(current_char())) {
        m_state = TokenizerState::NamedCharacterReference;
    } else if (current_char() == '#') {
        advance();
        m_state = TokenizerState::NumericCharacterReference;
    } else {
        // 不是有效的字符引用，作为普通字符处理
        m_state = m_return_state;
        if (m_return_state == TokenizerState::Data) {
            return create_text_token("&");
        }
        // 在属性值中
        m_token_builder.attr_value += '&';
    }
    return {};
}

std::optional<Token> Tokenizer::consume_named_character_reference_state() {
    std::string entity_name;
    size_t      start_pos = m_pos;

    // 收集实体名称
    while (has_more() && (is_alnum(current_char()))) {
        entity_name += current_char();
        advance();
    }

    // 检查分号
    bool has_semicolon = (current_char() == ';');
    if (has_semicolon) {
        advance();
    }

    // 查找字符引用
    auto it = NAMED_CHARACTER_REFERENCES.find(entity_name);
    if (it != NAMED_CHARACTER_REFERENCES.end()) {
        char replacement = it->second;
        m_state          = m_return_state;

        if (m_return_state == TokenizerState::Data) {
            return create_text_token(std::string(1, replacement));
        } else {
            // 在属性值中
            m_token_builder.attr_value += replacement;
        }
    } else {
        // 未找到匹配的实体，作为普通文本处理
        std::string fallback = "&" + entity_name;
        if (has_semicolon) {
            fallback += ";";
        }

        m_state = m_return_state;
        if (m_return_state == TokenizerState::Data) {
            return create_text_token(fallback);
        } else {
            m_token_builder.attr_value += fallback;
        }
    }
    return {};
}

std::optional<Token> Tokenizer::consume_numeric_character_reference_state() {
    bool is_hex = false;
    if (current_char() == 'x' || current_char() == 'X') {
        is_hex = true;
        advance();
    }

    std::string number;
    while (has_more() && (is_hex ? is_hex_digit(current_char()) : is_digit(current_char()))) {
        number += current_char();
        advance();
    }

    // 检查分号
    if (current_char() == ';') {
        advance();
    }

    if (number.empty()) {
        handle_parse_error(ParseException::ErrorCode::InvalidToken,
                           "Invalid numeric character reference");
        m_state = m_return_state;
        return {};
    }

    try {
        int code_point = std::stoi(number, nullptr, is_hex ? 16 : 10);

        // 基本的代码点验证
        if (code_point <= 0 || code_point > 0x10FFFF) {
            throw std::out_of_range("Invalid code point");
        }

        char replacement = static_cast<char>(code_point);
        m_state          = m_return_state;

        if (m_return_state == TokenizerState::Data) {
            return create_text_token(std::string(1, replacement));
        } else {
            m_token_builder.attr_value += replacement;
        }
    } catch (...) {
        handle_parse_error(ParseException::ErrorCode::InvalidToken,
                           "Invalid numeric character reference value");
        m_state = m_return_state;
    }

    return {};
}

std::optional<Token> Tokenizer::consume_markup_declaration_open_state() {
    if (starts_with("--")) {
        m_pos += 2;
        m_char_ref_buffer.clear();
        m_state = TokenizerState::Comment;
    } else if (starts_with("DOCTYPE") || starts_with("doctype")) {
        // 不区分大小写处理 DOCTYPE
        m_pos += 7;
        m_token_builder.reset();
        skip_whitespace();
        m_state = TokenizerState::DOCTYPEName;
    } else {
        handle_parse_error(ParseException::ErrorCode::InvalidToken, "Unknown markup declaration");
        // 跳过到 >
        while (has_more() && current_char() != '>') {
            advance();
        }
        if (current_char() == '>') {
            advance();
        }
        m_state = TokenizerState::Data;
    }
    return {};
}

std::optional<Token> Tokenizer::consume_cdata_section_state() {
    handle_parse_error(ParseException::ErrorCode::InvalidToken,
                       "CDATA sections not supported in HTML");
    m_state = TokenizerState::Data;
    return {};
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

bool Tokenizer::is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}
bool Tokenizer::is_hex_digit(char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

char Tokenizer::to_lower(char c) noexcept {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
}

bool Tokenizer::is_void_element_name(std::string_view n) noexcept {
    return VOID_ELEMENTS.contains(n);
}

Token Tokenizer::create_start_tag_token() {
    Token token(TokenType::OPEN, m_token_builder.tag_name, "");
    for (const auto& attr : m_token_builder.attrs) {
        token.add_attr(attr);
    }

    if (is_void_element_name(m_token_builder.tag_name)) {
        token.set_type(TokenType::CLOSE_SELF);
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

void Tokenizer::handle_parse_error(ParseException::ErrorCode code, const std::string& message) {
    record_error(code, message);

    switch (m_error_mode) {
        case ErrorHandlingMode::Strict:
            throw ParseException(code, message, m_pos);
        case ErrorHandlingMode::Lenient:
            transition_to_data_state();
            break;
        case ErrorHandlingMode::Ignore:
            break;
    }
}

void Tokenizer::record_error(ParseException::ErrorCode code, const std::string& message) {
    m_errors.emplace_back(code, message, m_pos);
}

void Tokenizer::transition_to_data_state() {
    m_state = TokenizerState::Data;
    m_token_builder.reset();
}

}  // namespace hps