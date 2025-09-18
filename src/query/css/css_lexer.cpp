#include "hps/query/css/css_lexer.hpp"

#include "hps/utils/exception.hpp"
#include "hps/utils/string_utils.hpp"

namespace hps {
// ==================== CSSLexer Implementation ====================

CSSLexer::CSSLexer(const std::string_view input)
    : m_input(input),
      m_position(0),
      m_line(1),
      m_column(1) {
    preprocess();
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
    if (safe_isspace(c)) {
        // 跳过连续的空白字符
        while (m_position < m_processed_input.size() && std::isspace(m_processed_input[m_position])) {
            update_position(m_processed_input[m_position]);
            m_position++;
        }
        return {CSSTokenType::Whitespace, " ", start_pos};
    }

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

        case '-': {
            advance();
            return {CSSTokenType::Minus, "-", start_pos};
        }

        case '"':
        case '\'': {
            std::string str = read_string(c);
            return {CSSTokenType::String, str, start_pos};
        }

        default: {
            if (safe_isdigit(c)) {
                std::string number = read_number();
                return {CSSTokenType::Number, number, start_pos};
            }

            if (is_valid_identifier_start(m_processed_input, m_position)) {
                std::string identifier = read_identifier();
                return {CSSTokenType::Identifier, identifier, start_pos};
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

            // 添加调试信息
            error_msg += " at position " + std::to_string(m_position);
            error_msg += ", processed_input size: " + std::to_string(m_processed_input.size());
            if (m_position < m_processed_input.size()) {
                error_msg += ", char code: " + std::to_string(static_cast<unsigned char>(c));
            }

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
}  // namespace hps