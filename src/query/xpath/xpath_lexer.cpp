#include "hps/query/xpath/xpath_lexer.hpp"

#include "hps/utils/string_utils.hpp"

namespace hps {

XPathLexer::XPathLexer(const std::string_view input)
    : m_input(input),
      m_position(0) {}

XPathToken XPathLexer::next_token() {
    if (m_peeked_token) {
        XPathToken result = std::move(*m_peeked_token);
        m_peeked_token.reset();
        return result;
    }

    skip_whitespace();

    if (m_position >= m_input.length()) {
        return {XPathTokenType::END_OF_FILE, "", m_position};
    }

    const char ch = current_char();

    // 处理字符串字面量
    if (ch == '"' || ch == '\'') {
        return scan_string_literal();
    }

    // 处理数字字面量
    if (is_digit(ch) || (ch == '.' && m_position + 1 < m_input.length() && is_digit(m_input[m_position + 1]))) {
        return scan_number_literal();
    }

    // 处理操作符
    XPathToken op_token = scan_operator();
    if (op_token.type != XPathTokenType::ERR) {
        return op_token;
    }

    // 处理标识符或关键字
    if (is_letter(ch) || ch == '_') {
        return scan_identifier_or_keyword();
    }

    // 未知字符
    const size_t error_position = m_position;  // 记录错误位置
    XPathToken   error_token(XPathTokenType::ERR, std::string(1, ch), error_position);
    advance();
    return error_token;
}

XPathToken XPathLexer::peek_token() const {
    if (!m_peeked_token) {
        // 创建一个副本用于预读
        XPathLexer lexer_copy(m_input);
        lexer_copy.m_position = m_position;
        if (m_peeked_token) {
            lexer_copy.m_peeked_token = std::make_unique<XPathToken>(*m_peeked_token);
        }

        m_peeked_token = std::make_unique<XPathToken>(lexer_copy.next_token());
    }
    return *m_peeked_token;
}

void XPathLexer::consume_token() {
    next_token();
}

bool XPathLexer::has_next() const {
    return m_position < m_input.length() || m_peeked_token;
}

size_t XPathLexer::position() const {
    return m_position;
}

char XPathLexer::current_char() const {
    if (m_position < m_input.length()) {
        return m_input[m_position];
    }
    return '\0';
}

char XPathLexer::peek_char() const {
    if (m_position + 1 < m_input.length()) {
        return m_input[m_position + 1];
    }
    return '\0';
}

void XPathLexer::advance() {
    if (m_position < m_input.length()) {
        m_position++;
    }
}

void XPathLexer::skip_whitespace() {
    while (m_position < m_input.length() && is_whitespace(current_char())) {
        advance();
    }
}

XPathToken XPathLexer::scan_string_literal() {
    const char   quote     = current_char();
    const size_t start_pos = m_position;  // 记录字符串开始位置
    advance();                            // 跳过开始引号

    std::string value;
    while (m_position < m_input.length() && current_char() != quote) {
        if (current_char() == '\\') {
            advance();  // 跳过反斜杠
            if (m_position < m_input.length()) {
                value += current_char();
                advance();
            }
        } else {
            value += current_char();
            advance();
        }
    }

    if (m_position < m_input.length()) {
        advance();  // 跳过结束引号
        return {XPathTokenType::STRING_LITERAL, value, start_pos};
    }
    // 未闭合的字符串 - 使用开始位置
    return {XPathTokenType::ERR, "Unclosed string literal", start_pos};
}

XPathToken XPathLexer::scan_number_literal() {
    const size_t start_pos = m_position;
    std::string  value;

    // 处理整数部分
    while (m_position < m_input.length() && is_digit(current_char())) {
        value += current_char();
        advance();
    }

    // 处理小数部分
    if (m_position < m_input.length() && current_char() == '.') {
        value += current_char();
        advance();

        while (m_position < m_input.length() && is_digit(current_char())) {
            value += current_char();
            advance();
        }
    }

    return {XPathTokenType::NUMBER_LITERAL, value, start_pos};
}

XPathToken XPathLexer::scan_identifier_or_keyword() {
    const size_t start_pos = m_position;
    std::string  value;

    while (m_position < m_input.length() && (is_letter(current_char()) || is_digit(current_char()) || current_char() == '_' || current_char() == '-')) {
        value += current_char();
        advance();
    }

    // 检查是否为逻辑操作符关键字
    if (value == "and") {
        return {XPathTokenType::AND, value, start_pos};
    }
    if (value == "or") {
        return {XPathTokenType::OR, value, start_pos};
    }
    if (value == "div") {
        return {XPathTokenType::DIV, value, start_pos};
    }
    if (value == "mod") {
        return {XPathTokenType::MOD, value, start_pos};
    }

    // 检查是否为轴标识符
    if (value == "ancestor") {
        return {XPathTokenType::AXIS_ANCESTOR, value, start_pos};
    }
    if (value == "ancestor-or-self") {
        return {XPathTokenType::AXIS_ANCESTOR_OR_SELF, value, start_pos};
    }
    if (value == "attribute") {
        return {XPathTokenType::AXIS_ATTRIBUTE, value, start_pos};
    }
    if (value == "child") {
        return {XPathTokenType::AXIS_CHILD, value, start_pos};
    }
    if (value == "descendant") {
        return {XPathTokenType::AXIS_DESCENDANT, value, start_pos};
    }
    if (value == "descendant-or-self") {
        return {XPathTokenType::AXIS_DESCENDANT_OR_SELF, value, start_pos};
    }
    if (value == "following") {
        return {XPathTokenType::AXIS_FOLLOWING, value, start_pos};
    }
    if (value == "following-sibling") {
        return {XPathTokenType::AXIS_FOLLOWING_SIBLING, value, start_pos};
    }
    if (value == "namespace") {
        return {XPathTokenType::AXIS_NAMESPACE, value, start_pos};
    }
    if (value == "parent") {
        return {XPathTokenType::AXIS_PARENT, value, start_pos};
    }
    if (value == "preceding") {
        return {XPathTokenType::AXIS_PRECEDING, value, start_pos};
    }
    if (value == "preceding-sibling") {
        return {XPathTokenType::AXIS_PRECEDING_SIBLING, value, start_pos};
    }
    if (value == "self") {
        return {XPathTokenType::AXIS_SELF, value, start_pos};
    }

    // 检查是否为节点类型测试
    if (value == "comment") {
        return {XPathTokenType::NODE_TYPE_COMMENT, value, start_pos};
    }
    if (value == "text") {
        return {XPathTokenType::NODE_TYPE_TEXT, value, start_pos};
    }
    if (value == "processing-instruction") {
        return {XPathTokenType::NODE_TYPE_PROCESSING_INSTRUCTION, value, start_pos};
    }
    if (value == "node") {
        return {XPathTokenType::NODE_TYPE_NODE, value, start_pos};
    }

    // 检查是否为XPath内置函数
    if (is_xpath_function(value)) {
        return {XPathTokenType::FUNCTION_NAME, value, start_pos};
    }

    // 检查下一个字符是否为左括号，如果是则可能是函数调用
    if (m_position < m_input.length() && current_char() == '(') {
        return {XPathTokenType::FUNCTION_NAME, value, start_pos};
    }

    // 普通标识符
    return {XPathTokenType::IDENTIFIER, value, start_pos};
}

// 私有辅助函数：检查是否为XPath内置函数
bool XPathLexer::is_xpath_function(const std::string& name) {
    // 节点集函数
    if (name == "last" || name == "position" || name == "count" || name == "id" || name == "local-name" || name == "namespace-uri" || name == "name") {
        return true;
    }

    // 字符串函数
    if (name == "string" || name == "concat" || name == "starts-with" || name == "contains" || name == "substring-before" || name == "substring-after" || name == "substring" || name == "string-length" || name == "normalize-space" || name == "translate") {
        return true;
    }

    // 布尔函数
    if (name == "boolean" || name == "not" || name == "true" || name == "false" || name == "lang") {
        return true;
    }

    // 数值函数
    if (name == "number" || name == "sum" || name == "floor" || name == "ceiling" || name == "round") {
        return true;
    }

    return false;
}

XPathToken XPathLexer::scan_operator() {
    const size_t start_pos = m_position;

    switch (current_char()) {
        case '/':
            advance();
            if (m_position < m_input.length() && current_char() == '/') {
                advance();
                return {XPathTokenType::DOUBLE_SLASH, "//", start_pos};
            }
            return {XPathTokenType::SLASH, "/", start_pos};

        case '.':
            advance();
            if (m_position < m_input.length() && current_char() == '.') {
                advance();
                return {XPathTokenType::DOUBLE_DOT, "..", start_pos};
            }
            return {XPathTokenType::DOT, ".", start_pos};

        case '@':
            advance();
            return {XPathTokenType::AT, "@", start_pos};

        case '*':
            advance();
            return {XPathTokenType::STAR, "*", start_pos};

        case '(':
            advance();
            return {XPathTokenType::LPAREN, "(", start_pos};

        case ')':
            advance();
            return {XPathTokenType::RPAREN, ")", start_pos};

        case '[':
            advance();
            return {XPathTokenType::LBRACKET, "[", start_pos};

        case ']':
            advance();
            return {XPathTokenType::RBRACKET, "]", start_pos};

        case '|':
            advance();
            return {XPathTokenType::PIPE, "|", start_pos};

        case ':':
            advance();
            if (m_position < m_input.length() && current_char() == ':') {
                advance();
                return {XPathTokenType::DOUBLE_COLON, "::", start_pos};
            }
            return {XPathTokenType::COLON, ":", start_pos};

        case '=':
            advance();
            return {XPathTokenType::EQUAL, "=", start_pos};

        case '!':
            advance();
            if (m_position < m_input.length() && current_char() == '=') {
                advance();
                return {XPathTokenType::NOT_EQUAL, "!=", start_pos};
            }
            // 错误处理
            return {XPathTokenType::ERR, "!", start_pos};

        case '<':
            advance();
            if (m_position < m_input.length() && current_char() == '=') {
                advance();
                return {XPathTokenType::LESS_EQUAL, "<=", start_pos};
            }
            return {XPathTokenType::LESS, "<", start_pos};

        case '>':
            advance();
            if (m_position < m_input.length() && current_char() == '=') {
                advance();
                return {XPathTokenType::GREATER_EQUAL, ">=", start_pos};
            }
            return {XPathTokenType::GREATER, ">", start_pos};

        case '+':
            advance();
            return {XPathTokenType::PLUS, "+", start_pos};

        case '-':
            advance();
            return {XPathTokenType::MINUS, "-", start_pos};

        case ',':
            advance();
            return {XPathTokenType::COMMA, ",", start_pos};

        default:
            // 不是操作符
            return {XPathTokenType::ERR, "", start_pos};
    }
}

}  // namespace hps