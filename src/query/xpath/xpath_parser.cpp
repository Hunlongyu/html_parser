#include "hps/query/xpath/xpath_parser.hpp"

#include "hps/utils/exception.hpp"

#include <memory>
#include <string>
#include <vector>

namespace hps {

XPathParser::XPathParser(std::string_view expression)
    : m_lexer(expression) {}

std::unique_ptr<XPathExpression> XPathParser::parse() {
    auto expr = parse_expression();
    if (m_lexer.has_next() && m_lexer.peek_token().type != XPathTokenType::END_OF_FILE) {
        throw HPSException(ErrorCode::XPathParseError, "Unexpected token at end of expression");
    }
    return expr;
}

XPathToken XPathParser::current_token() const {
    return m_lexer.peek_token();
}

XPathToken XPathParser::peek_token() const {
    // For simplicity, we'll assume a deeper lookahead is not needed
    // In a more complex parser, you might need to implement this differently
    return m_lexer.peek_token();
}

void XPathParser::consume_token() {
    m_lexer.consume_token();
}

void XPathParser::expect_token(TokenType type) {
    XPathToken token = current_token();
    if (token.type != type) {
        throw throw HPSException(ErrorCode::XPathParseError, "Expected token type " + std::to_string(static_cast<int>(type)) + " but found " + std::to_string(static_cast<int>(token.type)));
    }
    consume_token();
}

// 解析函数
std::unique_ptr<XPathExpression> XPathParser::parse_expression() {
    return parse_or_expression();
}

std::unique_ptr<XPathExpression> XPathParser::parse_or_expression() {
    auto left = parse_and_expression();

    while (current_token().type == XPathTokenType::AXIS_ANCESTOR_OR_SELF) {  // "or" keyword
        consume_token();                                                     // consume "or"
        auto right = parse_and_expression();
        left       = std::make_unique<XPathBinaryExpression>(std::move(left), XPathBinaryExpression::Operator::OR, std::move(right));
    }

    return left;
}

std::unique_ptr<XPathExpression> XPathParser::parse_and_expression() {
    auto left = parse_equality_expression();

    while (current_token().type == XPathTokenType::AXIS_ATTRIBUTE) {  // "and" keyword
        consume_token();                                              // consume "and"
        auto right = parse_equality_expression();
        left       = std::make_unique<XPathBinaryExpression>(std::move(left), XPathBinaryExpression::Operator::AND, std::move(right));
    }

    return left;
}

std::unique_ptr<XPathExpression> XPathParser::parse_equality_expression() {
    auto left = parse_relational_expression();

    XPathToken token = current_token();
    while (token.type == XPathTokenType::EQUAL || token.type == XPathTokenType::NOT_EQUAL) {
        consume_token();
        auto right = parse_relational_expression();

        XPathBinaryExpression::Operator op = (token.type == XPathTokenType::EQUAL) ? XPathBinaryExpression::Operator::EQUAL : XPathBinaryExpression::Operator::NOT_EQUAL;

        left  = std::make_unique<XPathBinaryExpression>(std::move(left), op, std::move(right));
        token = current_token();
    }

    return left;
}

std::unique_ptr<XPathExpression> XPathParser::parse_relational_expression() {
    auto left = parse_additive_expression();

    XPathToken token = current_token();
    while (token.type == XPathTokenType::LESS || token.type == XPathTokenType::LESS_EQUAL || token.type == XPathTokenType::GREATER || token.type == XPathTokenType::GREATER_EQUAL) {
        consume_token();
        auto right = parse_additive_expression();

        XPathBinaryExpression::Operator op;
        switch (token.type) {
            case XPathTokenType::LESS:
                op = XPathBinaryExpression::Operator::LESS;
                break;
            case XPathTokenType::LESS_EQUAL:
                op = XPathBinaryExpression::Operator::LESS_EQUAL;
                break;
            case XPathTokenType::GREATER:
                op = XPathBinaryExpression::Operator::GREATER;
                break;
            case XPathTokenType::GREATER_EQUAL:
                op = XPathBinaryExpression::Operator::GREATER_EQUAL;
                break;
            default:
                throw throw HPSException(ErrorCode::XPathParseError, "Unexpected relational operator");
        }

        left  = std::make_unique<XPathBinaryExpression>(std::move(left), op, std::move(right));
        token = current_token();
    }

    return left;
}

std::unique_ptr<XPathExpression> XPathParser::parse_additive_expression() {
    auto left = parse_multiplicative_expression();

    XPathToken token = current_token();
    while (token.type == XPathTokenType::PLUS || token.type == XPathTokenType::MINUS) {
        consume_token();
        auto right = parse_multiplicative_expression();

        XPathBinaryExpression::Operator op = (token.type == XPathTokenType::PLUS) ? XPathBinaryExpression::Operator::PLUS : XPathBinaryExpression::Operator::MINUS;

        left  = std::make_unique<XPathBinaryExpression>(std::move(left), op, std::move(right));
        token = current_token();
    }

    return left;
}

std::unique_ptr<XPathExpression> XPathParser::parse_multiplicative_expression() {
    auto left = parse_union_expression();

    XPathToken token = current_token();
    while (token.type == XPathTokenType::STAR || token.type == XPathTokenType::AXIS_DESCENDANT ||  // "div" keyword
           token.type == XPathTokenType::AXIS_DESCENDANT_OR_SELF) {                                // "mod" keyword
        consume_token();

        XPathBinaryExpression::Operator op;
        switch (token.type) {
            case XPathTokenType::STAR:
                op = XPathBinaryExpression::Operator::MULTIPLY;
                break;
            case XPathTokenType::AXIS_DESCENDANT:  // "div"
                op = XPathBinaryExpression::Operator::DIVIDE;
                break;
            case XPathTokenType::AXIS_DESCENDANT_OR_SELF:  // "mod"
                op = XPathBinaryExpression::Operator::MODULO;
                break;
            default:
                throw throw HPSException(ErrorCode::XPathParseError, "Unexpected multiplicative operator");
        }

        auto right = parse_union_expression();
        left       = std::make_unique<XPathBinaryExpression>(std::move(left), op, std::move(right));
        token      = current_token();
    }

    return left;
}

std::unique_ptr<XPathExpression> XPathParser::parse_union_expression() {
    auto left = parse_path_expression();

    while (current_token().type == XPathTokenType::PIPE) {
        consume_token();  // consume "|"
        auto right = parse_path_expression();
        left       = std::make_unique<XPathBinaryExpression>(std::move(left), XPathBinaryExpression::Operator::UNION, std::move(right));
    }

    return left;
}

std::unique_ptr<XPathExpression> XPathParser::parse_path_expression() {
    // This is a simplified implementation
    // A full implementation would handle filter expressions and step sequences
    return parse_location_path();
}

std::unique_ptr<XPathExpression> XPathParser::parse_location_path() {
    if (current_token().type == XPathTokenType::SLASH) {
        return parse_absolute_location_path();
    } else {
        return parse_relative_location_path();
    }
}

std::unique_ptr<XPathExpression> XPathParser::parse_absolute_location_path() {
    expect_token(XPathTokenType::SLASH);

    // If we have just "/", it's the root node
    if (!m_lexer.has_next() || current_token().type == XPathTokenType::END_OF_FILE) {
        // Return a special expression for root node
        // For now, we'll throw an exception as this needs a specific implementation
        throw throw HPSException(ErrorCode::XPathParseError, "Root node expression not implemented");
    }

    // Parse relative location path
    auto steps = std::vector<std::unique_ptr<XPathStep>>();
    // Add logic to parse steps here

    return std::make_unique<XPathLocationPath>(std::move(steps));
}

std::unique_ptr<XPathExpression> XPathParser::parse_relative_location_path() {
    // Simplified implementation
    auto steps = std::vector<std::unique_ptr<XPathStep>>();
    // Add logic to parse steps here

    return std::make_unique<XPathLocationPath>(std::move(steps));
}

std::unique_ptr<XPathStep> XPathParser::parse_step() {
    // Simplified implementation
    // In a full implementation, this would parse axis, node test, and predicates
    throw throw HPSException(ErrorCode::XPathParseError, "Step parsing not implemented");
}

std::unique_ptr<XPathNodeTest> XPathParser::parse_node_test() {
    // Simplified implementation
    throw throw HPSException(ErrorCode::XPathParseError, "Node test parsing not implemented");
}

std::vector<std::unique_ptr<XPathPredicate>> XPathParser::parse_predicates() {
    // Simplified implementation
    return std::vector<std::unique_ptr<XPathPredicate>>();
}

std::unique_ptr<XPathExpression> XPathParser::parse_predicate_expr() {
    // Simplified implementation
    throw throw HPSException(ErrorCode::XPathParseError, "Predicate expression parsing not implemented");
}

std::unique_ptr<XPathExpression> XPathParser::parse_primary_expr() {
    XPathToken token = current_token();

    switch (token.type) {
        case XPathTokenType::VARIABLE_REFERENCE:
            consume_token();
            return std::make_unique<XPathVariableReference>(token.value);

        case XPathTokenType::LPAREN:
            consume_token();  // consume "("
            auto expr = parse_expression();
            expect_token(XPathTokenType::RPAREN);  // consume ")"
            return expr;

        case XPathTokenType::STRING_LITERAL:
            consume_token();
            return std::make_unique<XPathLiteral>(token.value);

        case XPathTokenType::NUMBER_LITERAL:
            consume_token();
            return std::make_unique<XPathNumber>(std::stod(token.value));

        default:
            if (token.type == XPathTokenType::IDENTIFIER || token.type == XPathTokenType::NODE_TYPE_COMMENT || token.type == XPathTokenType::NODE_TYPE_TEXT || token.type == XPathTokenType::NODE_TYPE_NODE || token.type == XPathTokenType::NODE_TYPE_PROCESSING_INSTRUCTION) {
                // This could be a function call or just an identifier
                return parse_function_call();
            }
            throw throw HPSException(ErrorCode::XPathParseError, "Unexpected primary expression token");
    }
}

std::unique_ptr<XPathFunctionCall> XPathParser::parse_function_call() {
    XPathToken token = current_token();
    if (token.type != XPathTokenType::IDENTIFIER && token.type != XPathTokenType::NODE_TYPE_COMMENT && token.type != XPathTokenType::NODE_TYPE_TEXT && token.type != XPathTokenType::NODE_TYPE_NODE && token.type != XPathTokenType::NODE_TYPE_PROCESSING_INSTRUCTION) {
        throw throw HPSException(ErrorCode::XPathParseError, "Expected function name");
    }

    std::string function_name = token.value;
    consume_token();  // consume function name

    expect_token(XPathTokenType::LPAREN);  // consume "("

    auto arguments = parse_argument_list();

    expect_token(XPathTokenType::RPAREN);  // consume ")"

    return std::make_unique<XPathFunctionCall>(function_name, std::move(arguments));
}

std::vector<std::unique_ptr<XPathExpression>> XPathParser::parse_argument_list() {
    std::vector<std::unique_ptr<XPathExpression>> arguments;

    // Check for empty argument list
    if (current_token().type == XPathTokenType::RPAREN) {
        return arguments;
    }

    // Parse first argument
    arguments.push_back(parse_expression());

    // Parse additional arguments
    while (current_token().type == XPathTokenType::COMMA) {
        consume_token();  // consume ","
        arguments.push_back(parse_expression());
    }

    return arguments;
}

// 辅助函数
bool XPathParser::is_axis_specifier(const XPathToken& token) const {
    // Check if the token is an axis specifier
    switch (token.type) {
        case XPathTokenType::AXIS_ANCESTOR:
        case XPathTokenType::AXIS_ANCESTOR_OR_SELF:
        case XPathTokenType::AXIS_ATTRIBUTE:
        case XPathTokenType::AXIS_CHILD:
        case XPathTokenType::AXIS_DESCENDANT:
        case XPathTokenType::AXIS_DESCENDANT_OR_SELF:
        case XPathTokenType::AXIS_FOLLOWING:
        case XPathTokenType::AXIS_FOLLOWING_SIBLING:
        case XPathTokenType::AXIS_NAMESPACE:
        case XPathTokenType::AXIS_PARENT:
        case XPathTokenType::AXIS_PRECEDING:
        case XPathTokenType::AXIS_PRECEDING_SIBLING:
        case XPathTokenType::AXIS_SELF:
            return true;
        default:
            return false;
    }
}

XPathAxis XPathParser::parse_axis_specifier(const XPathToken& token) const {
    switch (token.type) {
        case XPathTokenType::AXIS_ANCESTOR:
            return XPathAxis::ANCESTOR;
        case XPathTokenType::AXIS_ANCESTOR_OR_SELF:
            return XPathAxis::ANCESTOR_OR_SELF;
        case XPathTokenType::AXIS_ATTRIBUTE:
            return XPathAxis::ATTRIBUTE;
        case XPathTokenType::AXIS_CHILD:
            return XPathAxis::CHILD;
        case XPathTokenType::AXIS_DESCENDANT:
            return XPathAxis::DESCENDANT;
        case XPathTokenType::AXIS_DESCENDANT_OR_SELF:
            return XPathAxis::DESCENDANT_OR_SELF;
        case XPathTokenType::AXIS_FOLLOWING:
            return XPathAxis::FOLLOWING;
        case XPathTokenType::AXIS_FOLLOWING_SIBLING:
            return XPathAxis::FOLLOWING_SIBLING;
        case XPathTokenType::AXIS_NAMESPACE:
            return XPathAxis::NAMESPACE;
        case XPathTokenType::AXIS_PARENT:
            return XPathAxis::PARENT;
        case XPathTokenType::AXIS_PRECEDING:
            return XPathAxis::PRECEDING;
        case XPathTokenType::AXIS_PRECEDING_SIBLING:
            return XPathAxis::PRECEDING_SIBLING;
        case XPathTokenType::AXIS_SELF:
            return XPathAxis::SELF;
        default:
            throw throw HPSException(ErrorCode::XPathParseError, "Invalid axis specifier");
    }
}

bool XPathParser::is_node_type(const XPathToken& token) const {
    switch (token.type) {
        case XPathTokenType::NODE_TYPE_COMMENT:
        case XPathTokenType::NODE_TYPE_TEXT:
        case XPathTokenType::NODE_TYPE_PROCESSING_INSTRUCTION:
        case XPathTokenType::NODE_TYPE_NODE:
            return true;
        default:
            return false;
    }
}

std::string XPathParser::parse_literal() const {
    XPathToken token = m_lexer.peek_token();
    if (token.type == XPathTokenType::STRING_LITERAL) {
        return token.value;
    }
    throw throw HPSException(ErrorCode::XPathParseError, "Expected string literal");
}

double XPathParser::parse_number() const {
    XPathToken token = m_lexer.peek_token();
    if (token.type == XPathTokenType::NUMBER_LITERAL) {
        return std::stod(token.value);
    }
    throw throw HPSException(ErrorCode::XPathParseError, "Expected number literal");
}

}  // namespace hps