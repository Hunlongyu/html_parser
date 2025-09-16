#include "hps/query/css/css_parser.hpp"

#include "hps/core/element.hpp"
#include "hps/utils/exception.hpp"

#include <algorithm>
#include <cctype>

namespace hps {

// ==================== CSSLexer Implementation ====================

CSSLexer::CSSLexer(std::string_view input) : m_input(input), m_position(0), m_line(1), m_column(1) {
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
        char c = m_input[i];

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

void CSSLexer::reset(std::string_view input) {
    m_input    = input;
    m_position = 0;
    m_line     = 1;
    m_column   = 1;
    m_current_token.reset();
    preprocess();
}

CSSLexer::CSSToken CSSLexer::read_next_token() {
    skip_whitespace();

    if (m_position >= m_processed_input.size()) {
        return CSSToken(CSSTokenType::EndOfFile, "", m_position);
    }

    char   c         = current_char();
    size_t start_pos = m_position;

    switch (c) {
        case '#': {
            advance();
            if (std::isalnum(current_char()) || current_char() == '_' || current_char() == '-') {
                std::string id = read_identifier();
                return CSSToken(CSSTokenType::Hash, id, start_pos);
            }
            throw_lexer_error("Invalid hash token");
        }

        case '.': {
            advance();
            return CSSToken(CSSTokenType::Dot, ".", start_pos);
        }

        case '*': {
            advance();
            if (current_char() == '=') {
                advance();
                return CSSToken(CSSTokenType::Contains, "*=", start_pos);
            }
            return CSSToken(CSSTokenType::Star, "*", start_pos);
        }

        case '[': {
            advance();
            return CSSToken(CSSTokenType::LeftBracket, "[", start_pos);
        }

        case ']': {
            advance();
            return CSSToken(CSSTokenType::RightBracket, "]", start_pos);
        }

        case '=': {
            advance();
            return CSSToken(CSSTokenType::Equals, "=", start_pos);
        }

        case '^': {
            advance();
            if (current_char() == '=') {
                advance();
                return CSSToken(CSSTokenType::StartsWith, "^=", start_pos);
            }
            throw_lexer_error("Unexpected character '^'");
        }

        case '$': {
            advance();
            if (current_char() == '=') {
                advance();
                return CSSToken(CSSTokenType::EndsWith, "$=", start_pos);
            }
            throw_lexer_error("Unexpected character '$'");
        }

        case '~': {
            advance();
            if (current_char() == '=') {
                advance();
                return CSSToken(CSSTokenType::WordMatch, "~=", start_pos);
            }
            return CSSToken(CSSTokenType::Tilde, "~", start_pos);
        }

        case '|': {
            advance();
            if (current_char() == '=') {
                advance();
                return CSSToken(CSSTokenType::LangMatch, "|=", start_pos);
            }
            throw_lexer_error("Unexpected character '|'");
        }

        case '>': {
            advance();
            return CSSToken(CSSTokenType::Greater, ">", start_pos);
        }

        case '+': {
            advance();
            return CSSToken(CSSTokenType::Plus, "+", start_pos);
        }

        case ',': {
            advance();
            return CSSToken(CSSTokenType::Comma, ",", start_pos);
        }

        case ':': {
            advance();
            if (current_char() == ':') {
                advance();
                return CSSToken(CSSTokenType::DoubleColon, "::", start_pos);
            }
            return CSSToken(CSSTokenType::Colon, ":", start_pos);
        }

        case '(': {
            advance();
            return CSSToken(CSSTokenType::LeftParen, "(", start_pos);
        }

        case ')': {
            advance();
            return CSSToken(CSSTokenType::RightParen, ")", start_pos);
        }

        case '"':
        case '\'': {
            std::string str = read_string(c);
            return CSSToken(CSSTokenType::String, str, start_pos);
        }

        default: {
            if (std::isalpha(c) || c == '_' || c == '-') {
                std::string identifier = read_identifier();
                return CSSToken(CSSTokenType::Identifier, identifier, start_pos);
            }

            if (std::isdigit(c)) {
                std::string number = read_number();
                return CSSToken(CSSTokenType::Identifier, number, start_pos);
            }

            throw_lexer_error("Unexpected character: '" + std::string(1, c) + "'");
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

void CSSLexer::skip_whitespace() {
    while (m_position < m_processed_input.size() && std::isspace(current_char())) {
        if (current_char() == ' ' || current_char() == '\t' || current_char() == '\n' || current_char() == '\r') {
            advance();
        } else {
            break;
        }
    }
}

std::string CSSLexer::read_identifier() {
    std::string result;
    while (m_position < m_processed_input.size()) {
        char c = current_char();
        if (std::isalnum(c) || c == '_' || c == '-') {
            result += c;
            advance();
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
        char c = current_char();

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
        char c = current_char();
        if (std::isdigit(c) || c == '.') {
            result += c;
            advance();
        } else {
            break;
        }
    }
    return result;
}

void CSSLexer::update_position(char c) {
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
        skip_whitespace();

        if (match_token(CSSLexer::CSSTokenType::Comma) || match_token(CSSLexer::CSSTokenType::EndOfFile)) {
            break;
        }

        SelectorType combinator = parse_combinator();
        if (combinator == SelectorType::Universal) {
            break;  // 没有找到组合器
        }

        skip_whitespace();
        auto right = parse_compound_selector();
        if (!right) {
            add_error("Expected selector after combinator");
            break;
        }

        // 创建组合选择器
        if (combinator == SelectorType::Descendant) {
            left = std::make_unique<DescendantSelector>(std::move(left), std::move(right));
        } else if (combinator == SelectorType::Child) {
            left = std::make_unique<ChildSelector>(std::move(left), std::move(right));
        }
        // 可以添加更多组合器类型的支持
    }

    return left;
}

std::unique_ptr<CSSSelector> CSSParser::parse_compound_selector() {
    auto compound = std::make_unique<CompoundSelector>();

    while (has_more_tokens()) {
        skip_whitespace();

        auto token = m_lexer.peek_token();
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
    auto token = m_lexer.peek_token();

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
            m_lexer.next_token();  // 消费 '*'
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

    auto attr_token = m_lexer.next_token();
    if (attr_token.type != CSSLexer::CSSTokenType::Identifier) {
        add_error("Expected attribute name in attribute selector");
        return nullptr;
    }

    std::string       attr_name = attr_token.value;
    AttributeOperator op        = AttributeOperator::Exists;
    std::string       value;

    // 检查是否有操作符
    auto next_token = m_lexer.peek_token();
    if (next_token.type != CSSLexer::CSSTokenType::RightBracket) {
        op = parse_attribute_operator();

        auto value_token = m_lexer.next_token();
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
    auto token = m_lexer.peek_token();

    if (token.type == CSSLexer::CSSTokenType::DoubleColon) {
        return parse_pseudo_element();
    } else if (token.type == CSSLexer::CSSTokenType::Colon) {
        return parse_pseudo_class();
    }

    return nullptr;
}

std::unique_ptr<CSSSelector> CSSParser::parse_pseudo_class() {
    consume_token(CSSLexer::CSSTokenType::Colon);

    auto name_token = m_lexer.next_token();
    if (name_token.type != CSSLexer::CSSTokenType::Identifier) {
        add_error("Expected pseudo-class name after ':'");
        return nullptr;
    }

    std::string name = name_token.value;
    std::string argument;

    // 检查是否有参数
    if (match_token(CSSLexer::CSSTokenType::LeftParen)) {
        consume_token(CSSLexer::CSSTokenType::LeftParen);

        // 读取参数（简化处理）
        auto arg_token = m_lexer.next_token();
        if (arg_token.type == CSSLexer::CSSTokenType::Identifier) {
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
    } else {
        add_error("Unsupported pseudo-class: " + name);
        return nullptr;
    }

    return std::make_unique<PseudoClassSelector>(type, argument);
}

std::unique_ptr<CSSSelector> CSSParser::parse_pseudo_element() {
    consume_token(CSSLexer::CSSTokenType::DoubleColon);

    auto name_token = m_lexer.next_token();
    if (name_token.type != CSSLexer::CSSTokenType::Identifier) {
        add_error("Expected pseudo-element name after '::'");
        return nullptr;
    }

    std::string name = name_token.value;

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
    auto token = m_lexer.peek_token();

    switch (token.type) {
        case CSSLexer::CSSTokenType::Greater:
            m_lexer.next_token();
            return SelectorType::Child;

        case CSSLexer::CSSTokenType::Plus:
            m_lexer.next_token();
            return SelectorType::Adjacent;

        case CSSLexer::CSSTokenType::Tilde:
            m_lexer.next_token();
            return SelectorType::Sibling;

        case CSSLexer::CSSTokenType::Whitespace:
            // 空白字符表示后代选择器
            return SelectorType::Descendant;

        default:
            // 检查是否有空白字符（通过位置变化检测）
            size_t old_pos = m_lexer.current_position();
            skip_whitespace();
            if (m_lexer.current_position() > old_pos) {
                return SelectorType::Descendant;
            }
            return SelectorType::Universal;  // 表示没有组合器
    }
}

AttributeOperator CSSParser::parse_attribute_operator() {
    auto token = m_lexer.next_token();

    switch (token.type) {
        case CSSLexer::CSSTokenType::Equals:
            return AttributeOperator::Equals;
        case CSSLexer::CSSTokenType::Contains:
            return AttributeOperator::Contains;
        case CSSLexer::CSSTokenType::StartsWith:
            return AttributeOperator::StartsWith;
        case CSSLexer::CSSTokenType::EndsWith:
            return AttributeOperator::EndsWith;
        case CSSLexer::CSSTokenType::WordMatch:
            return AttributeOperator::WordMatch;
        case CSSLexer::CSSTokenType::LangMatch:
            return AttributeOperator::LangMatch;
        default:
            add_error("Invalid attribute operator");
            return AttributeOperator::Exists;
    }
}

void CSSParser::consume_token(const CSSLexer::CSSTokenType expected) {
    auto token = m_lexer.next_token();
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
        auto token = m_lexer.next_token();
        if (token.type == CSSLexer::CSSTokenType::Comma || token.type == CSSLexer::CSSTokenType::EndOfFile) {
            return true;
        }
    }
    return false;
}

void CSSParser::throw_parse_error(const std::string& message) {
    throw HPSException(ErrorCode::InvalidSelector, "CSS Parser Error: " + message, SourceLocation(m_lexer.current_position(), m_lexer.current_line(), m_lexer.current_column()));
}

// ==================== PseudoClassSelector Implementation ====================

bool PseudoClassSelector::matches(const Element& element) const {
    switch (m_pseudo_type) {
        case PseudoType::FirstChild: {
            auto parent = element.parent();
            if (!parent)
                return false;

            auto siblings = parent->children();
            for (const auto& child : siblings) {
                if (child->type() == NodeType::Element) {
                    return child.get() == &element;
                }
            }
            return false;
        }

        case PseudoType::LastChild: {
            auto parent = element.parent();
            if (!parent)
                return false;

            auto siblings = parent->children();
            for (auto it = siblings.rbegin(); it != siblings.rend(); ++it) {
                if ((*it)->type() == NodeType::Element) {
                    return it->get() == &element;
                }
            }
            return false;
        }

        case PseudoType::Empty: {
            auto children = element.children();
            for (const auto& child : children) {
                if (child->type() == NodeType::Element || (child->type() == NodeType::Text && !child->text_content().empty())) {
                    return false;
                }
            }
            return true;
        }

        case PseudoType::Root: {
            return element.parent() == nullptr || element.parent()->type() == NodeType::Document;
        }

        // 状态伪类通常需要外部状态信息，这里简化处理
        case PseudoType::Hover:
        case PseudoType::Active:
        case PseudoType::Focus:
        case PseudoType::Visited:
        case PseudoType::Link:
        case PseudoType::Disabled:
        case PseudoType::Enabled:
        case PseudoType::Checked:
            return false;  // 需要外部状态支持

        default:
            return false;
    }
}

std::string PseudoClassSelector::to_string() const {
    switch (m_pseudo_type) {
        case PseudoType::FirstChild:
            return ":first-child";
        case PseudoType::LastChild:
            return ":last-child";
        case PseudoType::NthChild:
            return ":nth-child(" + m_argument + ")";
        case PseudoType::Empty:
            return ":empty";
        case PseudoType::Root:
            return ":root";
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

// ==================== PseudoElementSelector Implementation ====================

bool PseudoElementSelector::matches(const Element& element) const {
    // 伪元素通常不直接匹配DOM元素，而是用于样式应用
    // 这里返回false，实际应用中可能需要特殊处理
    return false;
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

// ==================== Utility Functions ====================

bool is_valid_selector(std::string_view selector) {
    try {
        CSSParser parser(selector);
        auto      result = parser.parse_selector_list();
        return result && !result->empty() && !parser.has_errors();
    } catch (const HPSException&) {
        return false;
    }
}

std::string normalize_selector(std::string_view selector) {
    // 移除多余空格，统一格式
    std::string result;
    result.reserve(selector.size());

    bool in_string      = false;
    char string_quote   = '\0';
    bool last_was_space = false;

    for (char c : selector) {
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
        } else if (std::isspace(c)) {
            if (!last_was_space && !result.empty()) {
                result += ' ';
                last_was_space = true;
            }
        } else {
            result += std::tolower(c);
            last_was_space = false;
        }
    }

    // 移除尾部空格
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }

    return result;
}

std::unique_ptr<SelectorList> parse_css_selector(std::string_view selector) {
    return parse_css_selector(selector, Options{});
}

std::unique_ptr<SelectorList> parse_css_selector(std::string_view selector, const Options& options) {
    std::string normalized = normalize_selector(selector);
    CSSParser   parser(selector, options);
    auto        result = parser.parse_selector_list();
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