#include "hps/query/xpath/xpath_lexer.hpp"

#include <cctype>
#include <unordered_set>

namespace hps {

//==============================================================================
// XPathLexer Implementation
//==============================================================================

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
    if (std::isdigit(ch) || (ch == '.' && m_position + 1 < m_input.length() && std::isdigit(m_input[m_position + 1]))) {
        return scan_number_literal();
    }

    // 处理变量引用
    if (ch == '$') {
        const size_t start_pos = m_position;
        advance();  // 跳过 $

        if (m_position < m_input.length() && (std::isalpha(current_char()) || current_char() == '_')) {
            auto identifier_token = scan_identifier_or_keyword();
            // 将标识符转换为变量引用
            return {XPathTokenType::VARIABLE, "$" + identifier_token.value, start_pos};
        } else {
            // $ 后面必须跟标识符
            return {XPathTokenType::ERR, "$", start_pos};
        }
    }

    // 处理操作符
    XPathToken op_token = scan_operator();
    if (op_token.type != XPathTokenType::ERR) {
        return op_token;
    }

    // 处理标识符或关键字
    if (std::isalpha(ch) || ch == '_') {
        return scan_identifier_or_keyword();
    }

    // 未知字符
    const size_t error_position = m_position;
    XPathToken   error_token(XPathTokenType::ERR, std::string(1, ch), error_position);
    advance();
    return error_token;
}

XPathToken XPathLexer::peek_token() const {
    if (!m_peeked_token) {
        // 保存当前状态
        size_t saved_position = m_position;

        // 创建临时 lexer 来获取下一个 token
        XPathLexer temp_lexer(m_input);
        temp_lexer.m_position = m_position;

        m_peeked_token = std::make_unique<XPathToken>(temp_lexer.next_token());
    }

    return *m_peeked_token;
}

void XPathLexer::consume_token() {
    if (m_peeked_token) {
        m_peeked_token.reset();
    } else {
        next_token();
    }
}

bool XPathLexer::has_next() const {
    return m_position < m_input.length();
}

size_t XPathLexer::position() const {
    return m_position;
}

//==============================================================================
// Character Operations
//==============================================================================

char XPathLexer::current_char() const {
    if (m_position >= m_input.length()) {
        return '\0';
    }
    return m_input[m_position];
}

char XPathLexer::peek_char() const {
    if (m_position + 1 >= m_input.length()) {
        return '\0';
    }
    return m_input[m_position + 1];
}

void XPathLexer::advance() {
    if (m_position < m_input.length()) {
        ++m_position;
    }
}

void XPathLexer::skip_whitespace() {
    while (m_position < m_input.length() && std::isspace(current_char())) {
        advance();
    }
}

//==============================================================================
// Scanning Methods
//==============================================================================

XPathToken XPathLexer::scan_string_literal() {
    const size_t start_pos = m_position;
    const char   quote     = current_char();

    advance();  // 跳过开始引号

    std::string value;
    while (m_position < m_input.length() && current_char() != quote) {
        if (current_char() == '\\' && m_position + 1 < m_input.length()) {
            // 处理转义字符
            advance();
            switch (current_char()) {
                case 'n':
                    value += '\n';
                    break;
                case 't':
                    value += '\t';
                    break;
                case 'r':
                    value += '\r';
                    break;
                case '\\':
                    value += '\\';
                    break;
                case '"':
                    value += '"';
                    break;
                case '\'':
                    value += '\'';
                    break;
                default:
                    value += '\\';
                    value += current_char();
                    break;
            }
        } else {
            value += current_char();
        }
        advance();
    }

    if (m_position >= m_input.length()) {
        // 未闭合的字符串
        return {XPathTokenType::ERR, "Unterminated string literal", start_pos};
    }

    advance();  // 跳过结束引号
    return {XPathTokenType::STRING_LITERAL, value, start_pos};
}

XPathToken XPathLexer::scan_number_literal() {
    const size_t start_pos = m_position;
    std::string  value;

    // 处理整数部分
    while (m_position < m_input.length() && std::isdigit(current_char())) {
        value += current_char();
        advance();
    }

    // 处理小数部分
    if (m_position < m_input.length() && current_char() == '.') {
        value += current_char();
        advance();

        while (m_position < m_input.length() && std::isdigit(current_char())) {
            value += current_char();
            advance();
        }
    }

    return {XPathTokenType::NUMBER_LITERAL, value, start_pos};
}

XPathToken XPathLexer::scan_identifier_or_keyword() {
    const size_t start_pos = m_position;
    std::string  value;

    // 扫描标识符字符
    while (m_position < m_input.length() && (std::isalnum(current_char()) || current_char() == '_' || current_char() == '-')) {
        value += current_char();
        advance();
    }

    // 检查是否是轴标识符
    if (m_position < m_input.length() && current_char() == ':' && m_position + 1 < m_input.length() && m_input[m_position + 1] == ':') {
        // 这是一个轴标识符
        if (value == "ancestor")
            return {XPathTokenType::AXIS_ANCESTOR, value + "::", start_pos};
        if (value == "ancestor-or-self")
            return {XPathTokenType::AXIS_ANCESTOR_OR_SELF, value + "::", start_pos};
        if (value == "attribute")
            return {XPathTokenType::AXIS_ATTRIBUTE, value + "::", start_pos};
        if (value == "child")
            return {XPathTokenType::AXIS_CHILD, value + "::", start_pos};
        if (value == "descendant")
            return {XPathTokenType::AXIS_DESCENDANT, value + "::", start_pos};
        if (value == "descendant-or-self")
            return {XPathTokenType::AXIS_DESCENDANT_OR_SELF, value + "::", start_pos};
        if (value == "following")
            return {XPathTokenType::AXIS_FOLLOWING, value + "::", start_pos};
        if (value == "following-sibling")
            return {XPathTokenType::AXIS_FOLLOWING_SIBLING, value + "::", start_pos};
        if (value == "namespace")
            return {XPathTokenType::AXIS_NAMESPACE, value + "::", start_pos};
        if (value == "parent")
            return {XPathTokenType::AXIS_PARENT, value + "::", start_pos};
        if (value == "preceding")
            return {XPathTokenType::AXIS_PRECEDING, value + "::", start_pos};
        if (value == "preceding-sibling")
            return {XPathTokenType::AXIS_PRECEDING_SIBLING, value + "::", start_pos};
        if (value == "self")
            return {XPathTokenType::AXIS_SELF, value + "::", start_pos};

        // 消费 ::
        advance();
        advance();
    }

    // 检查是否是节点类型测试
    if (m_position < m_input.length() && current_char() == '(') {
        if (value == "comment")
            return {XPathTokenType::NODE_TYPE_COMMENT, value, start_pos};
        if (value == "text")
            return {XPathTokenType::NODE_TYPE_TEXT, value, start_pos};
        if (value == "processing-instruction")
            return {XPathTokenType::NODE_TYPE_PROCESSING_INSTRUCTION, value, start_pos};
        if (value == "node")
            return {XPathTokenType::NODE_TYPE_NODE, value, start_pos};

        // 检查是否是函数调用
        if (is_xpath_function(value)) {
            return {XPathTokenType::FUNCTION_NAME, value, start_pos};
        }
    }

    // 检查是否是关键字
    if (value == "and")
        return {XPathTokenType::AND, value, start_pos};
    if (value == "or")
        return {XPathTokenType::OR, value, start_pos};
    if (value == "div")
        return {XPathTokenType::DIVIDE, value, start_pos};
    if (value == "mod")
        return {XPathTokenType::MODULO, value, start_pos};

    // 普通标识符
    return {XPathTokenType::IDENTIFIER, value, start_pos};
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
            return {XPathTokenType::UNION, "|", start_pos};

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

//==============================================================================
// Helper Functions
//==============================================================================

bool XPathLexer::is_xpath_function(const std::string& name) {
    static const std::unordered_set<std::string> xpath_functions = {// 节点集函数
                                                                    "last",
                                                                    "position",
                                                                    "count",
                                                                    "id",
                                                                    "local-name",
                                                                    "namespace-uri",
                                                                    "name",

                                                                    // 字符串函数
                                                                    "string",
                                                                    "concat",
                                                                    "starts-with",
                                                                    "contains",
                                                                    "substring-before",
                                                                    "substring-after",
                                                                    "substring",
                                                                    "string-length",
                                                                    "normalize-space",
                                                                    "translate",

                                                                    // 布尔函数
                                                                    "boolean",
                                                                    "not",
                                                                    "true",
                                                                    "false",
                                                                    "lang",

                                                                    // 数字函数
                                                                    "number",
                                                                    "sum",
                                                                    "floor",
                                                                    "ceiling",
                                                                    "round"};

    return xpath_functions.find(name) != xpath_functions.end();
}

}  // namespace hps