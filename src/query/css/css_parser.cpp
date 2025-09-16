#include "hps/query/css/css_parser.hpp"

#include "hps/core/element.hpp"
#include "hps/utils/exception.hpp"

#include <algorithm>
#include <cctype>
#include <codecvt>
#include <locale>
#include <regex>

namespace hps {

// ==================== CSSLexer Implementation ====================

CSSLexer::CSSLexer(const std::string_view input) : m_input(input), m_position(0), m_line(1), m_column(1) {
    preprocess();
}

// UTF-8 辅助函数
static bool is_utf8_start_byte(const unsigned char c) {
    return (c & 0x80) == 0 || (c & 0xE0) == 0xC0 || (c & 0xF0) == 0xE0 || (c & 0xF8) == 0xF0;
}

static int utf8_char_length(const unsigned char c) {
    if ((c & 0x80) == 0)
        return 1;  // 0xxxxxxx - ASCII
    if ((c & 0xE0) == 0xC0)
        return 2;  // 110xxxxx - 2字节UTF-8
    if ((c & 0xF0) == 0xE0)
        return 3;  // 1110xxxx - 3字节UTF-8
    if ((c & 0xF8) == 0xF0)
        return 4;  // 11110xxx - 4字节UTF-8
    return 1;      // 无效字节，按1字节处理
}

static bool is_valid_identifier_start(const std::string& input, const size_t pos) {
    if (pos >= input.size())
        return false;

    const unsigned char c = static_cast<unsigned char>(input[pos]);

    // ASCII字母、下划线、连字符
    if (std::isalpha(c) || c == '_' || c == '-') {
        return true;
    }

    // UTF-8多字节字符（中文等）
    if (c >= 0x80) {
        const int len = utf8_char_length(c);
        if (pos + len <= input.size()) {
            // 简单检查：如果是多字节UTF-8序列，认为是有效的标识符字符
            bool valid_sequence = true;
            for (int i = 1; i < len; ++i) {
                if (pos + i >= input.size() || (static_cast<unsigned char>(input[pos + i]) & 0xC0) != 0x80) {
                    valid_sequence = false;
                    break;
                }
            }
            return valid_sequence;
        }
    }

    return false;
}

static bool is_valid_identifier_char(const std::string& input, const size_t pos) {
    if (pos >= input.size())
        return false;

    const unsigned char c = static_cast<unsigned char>(input[pos]);

    // ASCII字母、数字、下划线、连字符
    if (std::isalnum(c) || c == '_' || c == '-') {
        return true;
    }

    // UTF-8多字节字符
    if (c >= 0x80) {
        const int len = utf8_char_length(c);
        if (pos + len <= input.size()) {
            bool valid_sequence = true;
            for (int i = 1; i < len; ++i) {
                if (pos + i >= input.size() || (static_cast<unsigned char>(input[pos + i]) & 0xC0) != 0x80) {
                    valid_sequence = false;
                    break;
                }
            }
            return valid_sequence;
        }
    }

    return false;
}

void CSSLexer::preprocess() {
    // 移除CSS注释并规范化空白字符
    m_processed_input.clear();
    m_processed_input.reserve(m_input.size());

    bool in_comment   = false;
    bool in_string    = false;
    char string_quote = '\0';

    for (size_t i = 0; i < m_input.size(); ++i) {
        const char c = m_input[i];

        if (!in_string && !in_comment && c == '/' && i + 1 < m_input.size() && m_input[i + 1] == '*') {
            in_comment = true;
            ++i;  // 跳过 '*'
            continue;
        }

        if (in_comment && c == '*' && i + 1 < m_input.size() && m_input[i + 1] == '/') {
            in_comment = false;
            ++i;  // 跳过 '/'
            continue;
        }

        if (in_comment) {
            continue;
        }

        if (!in_string && (c == '"' || c == '\'')) {
            in_string    = true;
            string_quote = c;
        } else if (in_string && c == string_quote && (i == 0 || m_input[i - 1] != '\\')) {
            in_string    = false;
            string_quote = '\0';
        }

        m_processed_input += c;
    }
}

CSSLexer::CSSToken CSSLexer::next_token() {
    if (m_current_token.has_value()) {
        CSSToken token = m_current_token.value();
        m_current_token.reset();
        return token;
    }
    return read_next_token();
}

CSSLexer::CSSToken CSSLexer::peek_token() {
    if (!m_current_token.has_value()) {
        m_current_token = read_next_token();
    }
    return m_current_token.value();
}

bool CSSLexer::has_more_tokens() const {
    return m_position < m_processed_input.size() || m_current_token.has_value();
}

void CSSLexer::reset(const std::string_view input) {
    m_input    = input;
    m_position = 0;
    m_line     = 1;
    m_column   = 1;
    m_current_token.reset();
    preprocess();
}

CSSLexer::CSSToken CSSLexer::read_next_token() {
    if (m_position >= m_processed_input.size()) {
        return {CSSTokenType::EndOfFile, "", m_position};
    }

    const char c         = current_char();
    size_t     start_pos = m_position;

    // 检查是否是空白字符，如果是则返回Whitespace token
    if (std::isspace(c)) {
        // 跳过连续的空白字符
        while (m_position < m_processed_input.size() && std::isspace(m_processed_input[m_position])) {
            update_position(m_processed_input[m_position]);
            m_position++;
        }
        return {CSSTokenType::Whitespace, " ", start_pos};
    }

    // 安全的字符类型检查 - 确保字符在有效范围内
    auto safe_isalnum = [](const char ch) -> bool { return static_cast<unsigned char>(ch) <= 255 && std::isalnum(static_cast<unsigned char>(ch)); };

    auto safe_isalpha = [](const char ch) -> bool { return static_cast<unsigned char>(ch) <= 255 && std::isalpha(static_cast<unsigned char>(ch)); };

    auto safe_isdigit = [](const char ch) -> bool { return static_cast<unsigned char>(ch) <= 255 && std::isdigit(static_cast<unsigned char>(ch)); };

    switch (c) {
        case '#': {
            advance();
            if (is_valid_identifier_start(m_processed_input, m_position)) {
                std::string id = read_identifier();
                return {CSSTokenType::Hash, id, start_pos};
            }
            throw_lexer_error("Invalid hash token");
        }

        case '.': {
            advance();
            return {CSSTokenType::Dot, ".", start_pos};
        }

        case '*': {
            advance();
            if (current_char() == '=') {
                advance();
                return {CSSTokenType::Contains, "*=", start_pos};
            }
            return {CSSTokenType::Star, "*", start_pos};
        }

        case '[': {
            advance();
            return {CSSTokenType::LeftBracket, "[", start_pos};
        }

        case ']': {
            advance();
            return {CSSTokenType::RightBracket, "]", start_pos};
        }

        case '=': {
            advance();
            return {CSSTokenType::Equals, "=", start_pos};
        }

        case '^': {
            advance();
            if (current_char() == '=') {
                advance();
                return {CSSTokenType::StartsWith, "^=", start_pos};
            }
            throw_lexer_error("Unexpected character '^'");
        }

        case '$': {
            advance();
            if (current_char() == '=') {
                advance();
                return {CSSTokenType::EndsWith, "$=", start_pos};
            }
            throw_lexer_error("Unexpected character '$'");
        }

        case '~': {
            advance();
            if (current_char() == '=') {
                advance();
                return {CSSTokenType::WordMatch, "~=", start_pos};
            }
            return {CSSTokenType::Tilde, "~", start_pos};
        }

        case '|': {
            advance();
            if (current_char() == '=') {
                advance();
                return {CSSTokenType::LangMatch, "|=", start_pos};
            }
            throw_lexer_error("Unexpected character '|'");
        }

        case '>': {
            advance();
            return {CSSTokenType::Greater, ">", start_pos};
        }

        case '+': {
            advance();
            return {CSSTokenType::Plus, "+", start_pos};
        }

        case ',': {
            advance();
            return {CSSTokenType::Comma, ",", start_pos};
        }

        case ':': {
            advance();
            if (current_char() == ':') {
                advance();
                return {CSSTokenType::DoubleColon, "::", start_pos};
            }
            return {CSSTokenType::Colon, ":", start_pos};
        }

        case '(': {
            advance();
            return {CSSTokenType::LeftParen, "(", start_pos};
        }

        case ')': {
            advance();
            return {CSSTokenType::RightParen, ")", start_pos};
        }

        case '"':
        case '\'': {
            std::string str = read_string(c);
            return {CSSTokenType::String, str, start_pos};
        }

        default: {
            if (is_valid_identifier_start(m_processed_input, m_position)) {
                std::string identifier = read_identifier();
                return {CSSTokenType::Identifier, identifier, start_pos};
            }

            if (safe_isdigit(c)) {
                std::string number = read_number();
                return {CSSTokenType::Identifier, number, start_pos};
            }

            // 对于无法识别的字符，提供更详细的错误信息
            std::string error_msg = "Unexpected character: '";
            if (static_cast<unsigned char>(c) >= 0x80) {
                // UTF-8字符，显示十六进制值
                error_msg += "\\x" + std::to_string(static_cast<unsigned char>(c));
            } else {
                error_msg += std::string(1, c);
            }
            error_msg += "'";
            throw_lexer_error(error_msg);
        }
    }
}

char CSSLexer::current_char() const {
    if (m_position >= m_processed_input.size()) {
        return '\0';
    }
    return m_processed_input[m_position];
}

char CSSLexer::peek_char(const size_t offset) const {
    const size_t pos = m_position + offset;
    if (pos >= m_processed_input.size()) {
        return '\0';
    }
    return m_processed_input[pos];
}

void CSSLexer::advance() {
    if (m_position < m_processed_input.size()) {
        update_position(m_processed_input[m_position]);
        ++m_position;
    }
}

bool CSSLexer::skip_whitespace() {
    bool skipped = false;
    while (m_position < m_processed_input.size() && std::isspace(m_processed_input[m_position])) {
        update_position(m_processed_input[m_position]);
        m_position++;
        skipped = true;
    }
    return skipped;
}

std::string CSSLexer::read_identifier() {
    std::string result;

    while (m_position < m_processed_input.size()) {
        if (is_valid_identifier_char(m_processed_input, m_position)) {
            const unsigned char c = static_cast<unsigned char>(m_processed_input[m_position]);

            if (c < 0x80) {
                // ASCII字符
                result += static_cast<char>(c);
                advance();
            } else {
                // UTF-8多字节字符
                const int len = utf8_char_length(c);
                for (int i = 0; i < len && m_position < m_processed_input.size(); ++i) {
                    result += m_processed_input[m_position];
                    advance();
                }
            }
        } else {
            break;
        }
    }
    return result;
}

std::string CSSLexer::read_string(const char quote) {
    advance();  // 跳过开始引号
    std::string result;

    while (m_position < m_processed_input.size()) {
        const char c = current_char();

        if (c == quote) {
            advance();  // 跳过结束引号
            break;
        }

        if (c == '\\' && peek_char() != '\0') {
            advance();  // 跳过反斜杠
            switch (const char escaped = current_char()) {
                case 'n':
                    result += '\n';
                    break;
                case 't':
                    result += '\t';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case '"':
                    result += '"';
                    break;
                case '\'':
                    result += '\'';
                    break;
                default:
                    result += escaped;
                    break;
            }
            advance();
        } else {
            result += c;
            advance();
        }
    }

    return result;
}

std::string CSSLexer::read_number() {
    std::string result;

    // 安全的数字字符检查
    auto safe_isdigit = [](const char ch) -> bool { return static_cast<unsigned char>(ch) <= 255 && std::isdigit(static_cast<unsigned char>(ch)); };

    while (m_position < m_processed_input.size()) {
        const char c = current_char();
        if (safe_isdigit(c) || c == '.') {
            result += c;
            advance();
        } else {
            break;
        }
    }
    return result;
}

void CSSLexer::update_position(const char c) {
    if (c == '\n') {
        ++m_line;
        m_column = 1;
    } else {
        ++m_column;
    }
}

void CSSLexer::throw_lexer_error(const std::string& message) const {
    throw HPSException(ErrorCode::InvalidSelector, "CSS Lexer Error at line " + std::to_string(m_line) + ", column " + std::to_string(m_column) + ": " + message, SourceLocation(m_position, m_line, m_column));
}

// ==================== CSSParser Implementation ====================

CSSParser::CSSParser(const std::string_view selector, const Options& options) : m_lexer(selector), m_options(options) {}

std::unique_ptr<SelectorList> CSSParser::parse_selector_list() {
    auto selector_list = std::make_unique<SelectorList>();

    do {
        skip_whitespace();
        if (!has_more_tokens()) {
            break;
        }

        if (auto selector = parse_selector()) {
            selector_list->add_selector(std::move(selector));
        }

        skip_whitespace();
        if (match_token(CSSLexer::CSSTokenType::Comma)) {
            consume_token(CSSLexer::CSSTokenType::Comma);
        } else {
            break;
        }
    } while (has_more_tokens());

    return selector_list;
}

std::unique_ptr<CSSSelector> CSSParser::parse_selector() {
    std::unique_ptr<CSSSelector> left = parse_compound_selector();
    if (!left) {
        return nullptr;
    }

    while (has_more_tokens()) {
        if (match_token(CSSLexer::CSSTokenType::Comma) || match_token(CSSLexer::CSSTokenType::EndOfFile)) {
            break;
        }

        // 检查是否有空白字符
        bool has_whitespace = false;
        while (match_token(CSSLexer::CSSTokenType::Whitespace)) {
            m_lexer.next_token();  // 消费空白字符
            has_whitespace = true;
        }

        // 检查显式组合器
        SelectorType combinator = SelectorType::Universal;
        const auto   token      = m_lexer.peek_token();

        if (token.type == CSSLexer::CSSTokenType::Greater) {
            m_lexer.next_token();
            combinator = SelectorType::Child;
        } else if (token.type == CSSLexer::CSSTokenType::Plus) {
            m_lexer.next_token();
            combinator = SelectorType::Adjacent;
        } else if (token.type == CSSLexer::CSSTokenType::Tilde) {
            m_lexer.next_token();
            combinator = SelectorType::Sibling;
        } else if (has_whitespace) {
            // 如果有空白字符但没有显式组合器，则为后代选择器
            combinator = SelectorType::Descendant;
        } else {
            // 没有空白字符也没有显式组合器，结束解析
            break;
        }

        // 跳过组合器后的空白字符
        while (match_token(CSSLexer::CSSTokenType::Whitespace)) {
            m_lexer.next_token();
        }

        auto right = parse_compound_selector();
        if (!right) {
            if (combinator != SelectorType::Descendant) {
                add_error("Expected selector after combinator");
            }
            break;
        }

        // 创建组合选择器
        switch (combinator) {
            case SelectorType::Descendant:
                left = std::make_unique<DescendantSelector>(std::move(left), std::move(right));
                break;
            case SelectorType::Child:
                left = std::make_unique<ChildSelector>(std::move(left), std::move(right));
                break;
            case SelectorType::Adjacent:
                left = std::make_unique<AdjacentSiblingSelector>(std::move(left), std::move(right));
                break;
            case SelectorType::Sibling:
                left = std::make_unique<GeneralSiblingSelector>(std::move(left), std::move(right));
                break;
            default:
                break;
        }
    }

    return left;
}

std::unique_ptr<CSSSelector> CSSParser::parse_compound_selector() {
    auto compound = std::make_unique<CompoundSelector>();

    while (has_more_tokens()) {
        const auto token = m_lexer.peek_token();
        if (token.type == CSSLexer::CSSTokenType::Greater || token.type == CSSLexer::CSSTokenType::Plus || token.type == CSSLexer::CSSTokenType::Tilde || token.type == CSSLexer::CSSTokenType::Comma || token.type == CSSLexer::CSSTokenType::EndOfFile) {
            break;
        }

        // 检查是否是空白字符（后代选择器）
        if (token.type == CSSLexer::CSSTokenType::Whitespace) {
            break;
        }

        if (auto simple_selector = parse_simple_selector()) {
            compound->add_selector(std::move(simple_selector));
        } else {
            break;
        }
    }

    if (compound->empty()) {
        return nullptr;
    }

    // 如果只有一个选择器，直接返回该选择器
    if (compound->selectors().size() == 1) {
        auto& selectors = const_cast<std::vector<std::unique_ptr<CSSSelector>>&>(compound->selectors());
        auto  result    = std::move(selectors[0]);
        return result;
    }

    return std::move(compound);
}

std::unique_ptr<CSSSelector> CSSParser::parse_simple_selector() {
    const auto token = m_lexer.peek_token();

    switch (token.type) {
        case CSSLexer::CSSTokenType::Identifier:
            return parse_type_selector();

        case CSSLexer::CSSTokenType::Dot:
            return parse_class_selector();

        case CSSLexer::CSSTokenType::Hash:
            return parse_id_selector();

        case CSSLexer::CSSTokenType::LeftBracket:
            return parse_attribute_selector();

        case CSSLexer::CSSTokenType::Star:
            m_lexer.next_token();
            return std::make_unique<UniversalSelector>();

        case CSSLexer::CSSTokenType::Colon:
        case CSSLexer::CSSTokenType::DoubleColon:
            return parse_pseudo_selector();

        default:
            return nullptr;
    }
}

std::unique_ptr<TypeSelector> CSSParser::parse_type_selector() {
    auto token = m_lexer.next_token();
    if (token.type != CSSLexer::CSSTokenType::Identifier) {
        add_error("Expected identifier for type selector");
        return nullptr;
    }
    return std::make_unique<TypeSelector>(token.value);
}

std::unique_ptr<ClassSelector> CSSParser::parse_class_selector() {
    consume_token(CSSLexer::CSSTokenType::Dot);

    auto token = m_lexer.next_token();
    if (token.type != CSSLexer::CSSTokenType::Identifier) {
        add_error("Expected identifier after '.' in class selector");
        return nullptr;
    }

    return std::make_unique<ClassSelector>(token.value);
}

std::unique_ptr<IdSelector> CSSParser::parse_id_selector() {
    auto token = m_lexer.next_token();
    if (token.type != CSSLexer::CSSTokenType::Hash) {
        add_error("Expected hash token for ID selector");
        return nullptr;
    }

    return std::make_unique<IdSelector>(token.value);
}

std::unique_ptr<AttributeSelector> CSSParser::parse_attribute_selector() {
    consume_token(CSSLexer::CSSTokenType::LeftBracket);

    const auto attr_token = m_lexer.next_token();
    if (attr_token.type != CSSLexer::CSSTokenType::Identifier) {
        add_error("Expected attribute name in attribute selector");
        return nullptr;
    }

    std::string       attr_name = attr_token.value;
    AttributeOperator op        = AttributeOperator::Exists;
    std::string       value;

    // 检查是否有操作符
    const auto next_token = m_lexer.peek_token();
    if (next_token.type != CSSLexer::CSSTokenType::RightBracket) {
        op = parse_attribute_operator();

        const auto value_token = m_lexer.next_token();
        if (value_token.type == CSSLexer::CSSTokenType::String || value_token.type == CSSLexer::CSSTokenType::Identifier) {
            value = value_token.value;
        } else {
            add_error("Expected value after attribute operator");
            return nullptr;
        }
    }

    consume_token(CSSLexer::CSSTokenType::RightBracket);

    return std::make_unique<AttributeSelector>(attr_name, op, value);
}

std::unique_ptr<CSSSelector> CSSParser::parse_pseudo_selector() {
    const auto token = m_lexer.peek_token();

    if (token.type == CSSLexer::CSSTokenType::DoubleColon) {
        return parse_pseudo_element();
    }
    if (token.type == CSSLexer::CSSTokenType::Colon) {
        return parse_pseudo_class();
    }

    return nullptr;
}

std::unique_ptr<CSSSelector> CSSParser::parse_pseudo_class() {
    consume_token(CSSLexer::CSSTokenType::Colon);

    const auto name_token = m_lexer.next_token();
    if (name_token.type != CSSLexer::CSSTokenType::Identifier) {
        add_error("Expected pseudo-class name after ':'");
        return nullptr;
    }

    const std::string name = name_token.value;
    std::string       argument;

    // 检查是否有参数
    if (match_token(CSSLexer::CSSTokenType::LeftParen)) {
        consume_token(CSSLexer::CSSTokenType::LeftParen);

        // 读取参数（简化处理）
        const auto arg_token = m_lexer.next_token();
        if (arg_token.type == CSSLexer::CSSTokenType::Identifier || arg_token.type == CSSLexer::CSSTokenType::String) {
            argument = arg_token.value;
        }

        consume_token(CSSLexer::CSSTokenType::RightParen);
    }

    // 映射伪类名称到类型
    PseudoClassSelector::PseudoType type;
    if (name == "first-child") {
        type = PseudoClassSelector::PseudoType::FirstChild;
    } else if (name == "last-child") {
        type = PseudoClassSelector::PseudoType::LastChild;
    } else if (name == "nth-child") {
        type = PseudoClassSelector::PseudoType::NthChild;
    } else if (name == "empty") {
        type = PseudoClassSelector::PseudoType::Empty;
    } else if (name == "root") {
        type = PseudoClassSelector::PseudoType::Root;
    } else if (name == "hover") {
        type = PseudoClassSelector::PseudoType::Hover;
    } else if (name == "active") {
        type = PseudoClassSelector::PseudoType::Active;
    } else if (name == "focus") {
        type = PseudoClassSelector::PseudoType::Focus;
    } else if (name == "disabled") {
        type = PseudoClassSelector::PseudoType::Disabled;
    } else if (name == "enabled") {
        type = PseudoClassSelector::PseudoType::Enabled;
    } else if (name == "checked") {
        type = PseudoClassSelector::PseudoType::Checked;
    } else if (name == "visited") {
        type = PseudoClassSelector::PseudoType::Visited;
    } else if (name == "link") {
        type = PseudoClassSelector::PseudoType::Link;
    } else {
        add_error("Unsupported pseudo-class: " + name);
        return nullptr;
    }

    return std::make_unique<PseudoClassSelector>(type, argument);
}

std::unique_ptr<CSSSelector> CSSParser::parse_pseudo_element() {
    consume_token(CSSLexer::CSSTokenType::DoubleColon);

    const auto name_token = m_lexer.next_token();
    if (name_token.type != CSSLexer::CSSTokenType::Identifier) {
        add_error("Expected pseudo-element name after '::'");
        return nullptr;
    }

    const std::string name = name_token.value;

    // 映射伪元素名称到类型
    PseudoElementSelector::ElementType type;
    if (name == "before") {
        type = PseudoElementSelector::ElementType::Before;
    } else if (name == "after") {
        type = PseudoElementSelector::ElementType::After;
    } else if (name == "first-line") {
        type = PseudoElementSelector::ElementType::FirstLine;
    } else if (name == "first-letter") {
        type = PseudoElementSelector::ElementType::FirstLetter;
    } else {
        add_error("Unsupported pseudo-element: " + name);
        return nullptr;
    }

    return std::make_unique<PseudoElementSelector>(type);
}

SelectorType CSSParser::parse_combinator() {
    const auto token = m_lexer.peek_token();

    // 检查空白字符（后代选择器）
    if (token.type == CSSLexer::CSSTokenType::Whitespace) {
        m_lexer.next_token();  // 消费空白字符token
        return SelectorType::Descendant;
    }

    if (token.type == CSSLexer::CSSTokenType::Greater) {
        m_lexer.next_token();
        return SelectorType::Child;
    }
    if (token.type == CSSLexer::CSSTokenType::Plus) {
        m_lexer.next_token();
        return SelectorType::Adjacent;
    }
    if (token.type == CSSLexer::CSSTokenType::Tilde) {
        m_lexer.next_token();
        return SelectorType::Sibling;
    }
    return SelectorType::Universal;
}

AttributeOperator CSSParser::parse_attribute_operator() {
    if (match_token(CSSLexer::CSSTokenType::Equals)) {
        m_lexer.next_token();
        return AttributeOperator::Equals;
    }
    if (match_token(CSSLexer::CSSTokenType::Contains)) {
        m_lexer.next_token();
        return AttributeOperator::Contains;
    }
    if (match_token(CSSLexer::CSSTokenType::StartsWith)) {
        m_lexer.next_token();
        return AttributeOperator::StartsWith;
    }
    if (match_token(CSSLexer::CSSTokenType::EndsWith)) {
        m_lexer.next_token();
        return AttributeOperator::EndsWith;
    }
    if (match_token(CSSLexer::CSSTokenType::WordMatch)) {
        m_lexer.next_token();
        return AttributeOperator::WordMatch;
    }
    if (match_token(CSSLexer::CSSTokenType::LangMatch)) {
        m_lexer.next_token();
        return AttributeOperator::LangMatch;
    }

    return AttributeOperator::Exists;
}

void CSSParser::consume_token(const CSSLexer::CSSTokenType expected) {
    const auto token = m_lexer.next_token();
    if (token.type != expected) {
        add_error("Unexpected token", token);
    }
}

bool CSSParser::match_token(const CSSLexer::CSSTokenType type) {
    return m_lexer.peek_token().type == type;
}

void CSSParser::skip_whitespace() {
    while (has_more_tokens() && match_token(CSSLexer::CSSTokenType::Whitespace)) {
        m_lexer.next_token();
    }
}

bool CSSParser::has_more_tokens() {
    return m_lexer.has_more_tokens() && !match_token(CSSLexer::CSSTokenType::EndOfFile);
}

void CSSParser::add_error(const std::string& message) {
    m_errors.emplace_back(ErrorCode::InvalidSelector, message, m_lexer.current_position());

    if (m_options.error_handling == ErrorHandlingMode::Strict) {
        throw_parse_error(message);
    }
}

void CSSParser::add_error(const std::string& message, const CSSLexer::CSSToken& token) {
    m_errors.emplace_back(ErrorCode::InvalidSelector, message, token.position);

    if (m_options.error_handling == ErrorHandlingMode::Strict) {
        throw_parse_error(message);
    }
}

bool CSSParser::try_recover_from_error() {
    // 简单的错误恢复：跳到下一个逗号或结束
    while (has_more_tokens()) {
        const auto token = m_lexer.next_token();
        if (token.type == CSSLexer::CSSTokenType::Comma || token.type == CSSLexer::CSSTokenType::EndOfFile) {
            return true;
        }
    }
    return false;
}

void CSSParser::throw_parse_error(const std::string& message) const {
    throw HPSException(ErrorCode::InvalidSelector, "CSS Parser Error: " + message, SourceLocation(m_lexer.current_position(), m_lexer.current_line(), m_lexer.current_column()));
}

// ==================== PseudoClassSelector Implementation ====================

bool PseudoClassSelector::matches(const Element& element) const {
    switch (m_pseudo_type) {
        case PseudoType::FirstChild: {
            // :first-child - 检查是否为父元素的第一个子元素
            auto parent = element.parent();
            if (!parent) {
                return false;
            }

            auto siblings = parent->children();
            for (const auto& child : siblings) {
                if (child->type() == NodeType::Element) {
                    return child.get() == &element;
                }
            }
            return false;
        }

        case PseudoType::LastChild: {
            // :last-child - 检查是否为父元素的最后一个子元素
            auto parent = element.parent();
            if (!parent) {
                return false;
            }

            auto siblings = parent->children();
            for (auto it = siblings.rbegin(); it != siblings.rend(); ++it) {
                if ((*it)->type() == NodeType::Element) {
                    return it->get() == &element;
                }
            }
            return false;
        }

        case PseudoType::NthChild: {
            // :nth-child(n) - 检查是否为第n个子元素
            auto parent = element.parent();
            if (!parent) {
                return false;
            }

            int  index    = 1;
            auto siblings = parent->children();
            for (const auto& child : siblings) {
                if (child->type() == NodeType::Element) {
                    if (child.get() == &element) {
                        return matches_nth_expression(m_argument, index);
                    }
                    index++;
                }
            }
            return false;
        }

        case PseudoType::NthLastChild: {
            // :nth-last-child(n) - 检查是否为倒数第n个子元素
            auto parent = element.parent();
            if (!parent) {
                return false;
            }

            // 先收集所有元素子节点
            std::vector<const Node*> element_children;
            for (auto siblings = parent->children(); const auto& child : siblings) {
                if (child->type() == NodeType::Element) {
                    element_children.push_back(child.get());
                }
            }

            // 从末尾开始计算索引
            for (int i = element_children.size() - 1; i >= 0; i--) {
                if (element_children[i] == &element) {
                    int reverse_index = element_children.size() - i;
                    return matches_nth_expression(m_argument, reverse_index);
                }
            }
            return false;
        }

        case PseudoType::FirstOfType: {
            // :first-of-type - 检查是否为同标签名的第一个元素
            return get_type_index(element) == 1;
        }

        case PseudoType::LastOfType: {
            // :last-of-type - 检查是否为同标签名的最后一个元素
            return get_type_index(element, true) == 1;
        }

        case PseudoType::OnlyChild: {
            // :only-child - 检查是否为唯一的子元素
            auto parent = element.parent();
            if (!parent) {
                return false;
            }

            int element_count = 0;
            for (auto siblings = parent->children(); const auto& child : siblings) {
                if (child->type() == NodeType::Element) {
                    element_count++;
                    if (element_count > 1) {
                        return false;
                    }
                }
            }
            return element_count == 1;
        }

        case PseudoType::OnlyOfType: {
            // :only-of-type - 检查是否为同标签名的唯一元素
            return count_siblings_of_type(element) == 1;
        }

        case PseudoType::Empty: {
            // :empty - 检查元素是否为空（无子元素和文本内容）
            auto children = element.children();

            // 安全的空白字符检查函数
            auto safe_isspace = [](const char ch) -> bool { return static_cast<unsigned char>(ch) <= 255 && std::isspace(static_cast<unsigned char>(ch)); };

            for (const auto& child : children) {
                if (child->type() == NodeType::Element) {
                    return false;
                }
                if (child->type() == NodeType::Text) {
                    auto text_content = child->text_content();
                    // 检查是否只包含空白字符
                    if (!text_content.empty() && !std::ranges::all_of(text_content, safe_isspace)) {
                        return false;
                    }
                }
            }
            return true;
        }

        case PseudoType::Root: {
            // :root - 检查是否为根元素
            auto parent = element.parent();
            return parent == nullptr || parent->type() == NodeType::Document;
        }

        case PseudoType::Not: {
            // :not(selector) - 否定伪类，需要额外的选择器参数
            // 这里简化处理，实际需要解析内部选择器
            return false;
        }

        // 状态伪类通常需要外部状态信息，这里提供基础实现
        case PseudoType::Hover:
        case PseudoType::Active:
        case PseudoType::Focus:
            // 这些状态需要外部状态管理器支持
            return false;

        case PseudoType::Visited:
        case PseudoType::Link: {
            // 链接相关伪类，检查是否为a标签且有href属性
            if (element.tag_name() != "a") {
                return false;
            }
            bool has_href = element.has_attribute("href");
            return (m_pseudo_type == PseudoType::Link) ? has_href : false;
        }

        case PseudoType::Disabled: {
            // :disabled - 检查表单元素是否被禁用
            return element.has_attribute("disabled");
        }

        case PseudoType::Enabled: {
            // :enabled - 检查表单元素是否启用
            const std::vector<std::string> form_elements   = {"input", "button", "select", "textarea", "option", "optgroup", "fieldset"};
            const auto&                    tag             = element.tag_name();
            bool                           is_form_element = std::ranges::find(form_elements, tag) != form_elements.end();
            return is_form_element && !element.has_attribute("disabled");
        }

        case PseudoType::Checked: {
            // :checked - 检查表单元素是否被选中
            if (const auto& tag = element.tag_name(); tag == "input") {
                if (auto type = element.get_attribute("type"); type == "checkbox" || type == "radio") {
                    return element.has_attribute("checked");
                }
            } else if (tag == "option") {
                return element.has_attribute("selected");
            }
            return false;
        }
    }
    return false;
}

std::string PseudoClassSelector::to_string() const {
    switch (m_pseudo_type) {
        case PseudoType::FirstChild:
            return ":first-child";
        case PseudoType::LastChild:
            return ":last-child";
        case PseudoType::NthChild:
            return ":nth-child(" + m_argument + ")";
        case PseudoType::NthLastChild:
            return ":nth-last-child(" + m_argument + ")";
        case PseudoType::FirstOfType:
            return ":first-of-type";
        case PseudoType::LastOfType:
            return ":last-of-type";
        case PseudoType::OnlyChild:
            return ":only-child";
        case PseudoType::OnlyOfType:
            return ":only-of-type";
        case PseudoType::Empty:
            return ":empty";
        case PseudoType::Root:
            return ":root";
        case PseudoType::Not:
            return ":not(" + m_argument + ")";
        case PseudoType::Hover:
            return ":hover";
        case PseudoType::Active:
            return ":active";
        case PseudoType::Focus:
            return ":focus";
        case PseudoType::Visited:
            return ":visited";
        case PseudoType::Link:
            return ":link";
        case PseudoType::Disabled:
            return ":disabled";
        case PseudoType::Enabled:
            return ":enabled";
        case PseudoType::Checked:
            return ":checked";
        default:
            return ":unknown";
    }
}

bool PseudoClassSelector::matches_nth_expression(const std::string& expression, const int index) {
    if (expression.empty()) {
        return false;
    }

    // 处理特殊关键字
    if (expression == "odd") {
        return index % 2 == 1;
    }
    if (expression == "even") {
        return index % 2 == 0;
    }

    // 安全的数字字符检查
    auto safe_isdigit = [](const char ch) -> bool { return static_cast<unsigned char>(ch) <= 255 && std::isdigit(static_cast<unsigned char>(ch)); };

    // 处理纯数字
    if (std::ranges::all_of(expression, safe_isdigit)) {
        return index == std::stoi(expression);
    }

    // 处理an+b格式
    // 简化的解析器，支持如"2n+1"、"3n"、"-n+6"等格式
    const std::regex nth_regex(R"(^\s*([+-]?\d*)n\s*([+-]\s*\d+)?\s*$)");
    std::smatch      match;

    if (std::regex_match(expression, match, nth_regex)) {
        int a = 1;  // 默认系数
        int b = 0;  // 默认偏移

        // 解析系数a
        const std::string a_str = match[1].str();
        if (!a_str.empty()) {
            if (a_str == "+" || a_str == "") {
                a = 1;
            } else if (a_str == "-") {
                a = -1;
            } else {
                a = std::stoi(a_str);
            }
        }

        // 解析偏移b
        std::string b_str = match[2].str();
        if (!b_str.empty()) {
            // 移除空格
            b_str.erase(std::ranges::remove_if(b_str, ::isspace).begin(), b_str.end());
            b = std::stoi(b_str);
        }

        // 检查是否匹配an+b公式
        if (a == 0) {
            return index == b;
        }

        // 对于正系数，检查(index - b) % a == 0 且 index >= b
        // 对于负系数，需要特殊处理
        if (a > 0) {
            return index >= b && (index - b) % a == 0;
        }
        return index <= b && (b - index) % (-a) == 0;
    }

    return false;
}

int PseudoClassSelector::count_siblings_of_type(const Element& element) {
    const auto parent = element.parent();
    if (!parent) {
        return 1;
    }

    int         count    = 0;
    const auto& tag_name = element.tag_name();
    const auto  siblings = parent->children();

    for (const auto& child : siblings) {
        if (child->type() == NodeType::Element) {
            const auto child_element = child->as_element();
            if (child_element && child_element->tag_name() == tag_name) {
                count++;
            }
        }
    }

    return count;
}

int PseudoClassSelector::get_type_index(const Element& element, const bool from_end) {
    const auto parent = element.parent();
    if (!parent) {
        return 1;
    }

    const auto&                 tag_name = element.tag_name();
    const auto                  siblings = parent->children();
    std::vector<const Element*> same_type_elements;

    // 收集所有同类型的元素
    for (const auto& child : siblings) {
        if (child->type() == NodeType::Element) {
            auto child_element = child->as_element();
            if (child_element && child_element->tag_name() == tag_name) {
                same_type_elements.push_back(child_element.get());
            }
        }
    }

    // 查找目标元素的索引
    for (size_t i = 0; i < same_type_elements.size(); i++) {
        if (same_type_elements[i] == &element) {
            if (from_end) {
                return same_type_elements.size() - i;
            } else {
                return i + 1;
            }
        }
    }

    return 0;
}

std::string PseudoElementSelector::to_string() const {
    switch (m_element_type) {
        case ElementType::Before:
            return "::before";
        case ElementType::After:
            return "::after";
        case ElementType::FirstLine:
            return "::first-line";
        case ElementType::FirstLetter:
            return "::first-letter";
        default:
            return "::unknown";
    }
}

/**
 * @brief 伪元素选择器匹配实现
 *
 * 伪元素选择器通常不直接匹配DOM元素，而是用于样式应用。
 * 在实际的CSS引擎中，伪元素会创建虚拟元素来应用样式。
 * 这里我们提供一个基础实现，总是返回false，因为DOM中不存在真实的伪元素。
 */
bool PseudoElementSelector::matches(const Element& element) const {
    // 伪元素选择器不直接匹配DOM元素
    // 在真实的CSS引擎中，伪元素用于创建虚拟元素来应用样式
    // 例如 ::before 和 ::after 会在元素内容前后插入虚拟内容
    // ::first-line 和 ::first-letter 会对文本的特定部分应用样式

    // 对于查询目的，我们可以检查元素是否支持该伪元素
    switch (m_element_type) {
        case ElementType::Before:
        case ElementType::After:
            // ::before 和 ::after 可以应用于大多数元素
            // 但通常不应用于替换元素（如 img, input 等）
            {
                const std::string& tag = element.tag_name();
                // 排除不支持 ::before/::after 的替换元素
                static const std::vector<std::string> replaced_elements = {"img", "input", "textarea", "select", "option", "br", "hr", "area", "base", "col", "embed", "link", "meta", "param", "source", "track", "wbr"};
                return std::ranges::find(replaced_elements, tag) == replaced_elements.end();
            }

        case ElementType::FirstLine:
        case ElementType::FirstLetter:
            // ::first-line 和 ::first-letter 只能应用于块级元素
            // 这里简化处理，检查是否为常见的块级元素
            {
                const std::string&                    tag            = element.tag_name();
                static const std::vector<std::string> block_elements = {"div", "p", "h1", "h2", "h3", "h4", "h5", "h6", "article", "section", "header", "footer", "main", "aside", "nav", "blockquote", "pre", "address"};
                return std::ranges::find(block_elements, tag) != block_elements.end();
            }

        default:
            return false;
    }
}

// ==================== Utility Functions ====================

bool is_valid_selector(const std::string_view selector) {
    try {
        CSSParser  parser(selector);
        const auto result = parser.parse_selector_list();
        return result && !result->empty() && !parser.has_errors();
    } catch (const HPSException&) {
        return false;
    }
}

std::string normalize_selector(const std::string_view selector) {
    // 移除多余空格，统一格式
    std::string result;
    result.reserve(selector.size());

    bool in_string      = false;
    char string_quote   = '\0';
    bool last_was_space = false;

    // 安全的字符类型检查函数
    auto safe_isspace = [](const char ch) -> bool { return static_cast<unsigned char>(ch) <= 255 && std::isspace(static_cast<unsigned char>(ch)); };

    auto safe_tolower = [](const char ch) -> char { return static_cast<unsigned char>(ch) <= 255 ? std::tolower(static_cast<unsigned char>(ch)) : ch; };

    for (const char c : selector) {
        if (!in_string && (c == '"' || c == '\'')) {
            in_string    = true;
            string_quote = c;
            result += c;
            last_was_space = false;
        } else if (in_string && c == string_quote) {
            in_string    = false;
            string_quote = '\0';
            result += c;
            last_was_space = false;
        } else if (in_string) {
            result += c;
            last_was_space = false;
        } else if (safe_isspace(c)) {
            if (!last_was_space && !result.empty()) {
                result += ' ';
                last_was_space = true;
            }
        } else {
            result += safe_tolower(c);
            last_was_space = false;
        }
    }

    // 移除尾部空格
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }

    return result;
}

std::unique_ptr<SelectorList> parse_css_selector(const std::string_view selector) {
    return parse_css_selector(selector, Options{});
}

std::unique_ptr<SelectorList> parse_css_selector(const std::string_view selector, const Options& options) {
    const std::string normalized = normalize_selector(selector);
    CSSParser         parser(normalized, options);  // 使用规范化后的选择器字符串
    auto              result = parser.parse_selector_list();
    if (parser.has_errors() && options.error_handling == ErrorHandlingMode::Strict) {
        throw HPSException(ErrorCode::InvalidSelector, "CSS parsing failed");
    }
    return result;
}

std::vector<std::unique_ptr<SelectorList>> parse_css_selectors(const std::vector<std::string_view>& selectors) {
    std::vector<std::unique_ptr<SelectorList>> results;
    results.reserve(selectors.size());

    for (const auto& selector : selectors) {
        try {
            auto result = parse_css_selector(selector);
            results.push_back(std::move(result));
        } catch (const HPSException&) {
            results.push_back(nullptr);
        }
    }

    return results;
}

}  // namespace hps