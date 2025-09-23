#include "hps/query/xpath/xpath_parser.hpp"

#include <algorithm>
#include <sstream>

namespace hps {

//==============================================================================
// XPathParseError Implementation
//==============================================================================

// 异常类已在头文件中内联实现

//==============================================================================
// XPathParser Implementation
//==============================================================================

XPathParser::XPathParser(const std::string_view expression)
    : m_lexer(expression),
      m_token_consumed(true) {}

std::unique_ptr<XPathExpression> XPathParser::parse() {
    // 开始解析，获取第一个 token
    advance();

    if (current_token().type == XPathTokenType::END_OF_FILE) {
        throw_parse_error("Empty expression");
    }

    auto result = parse_or_expression();

    // 确保所有 token 都被消费
    if (current_token().type != XPathTokenType::END_OF_FILE) {
        throw_parse_error("Unexpected token after expression");
    }

    return result;
}

//==============================================================================
// Token Management
//==============================================================================

const XPathToken& XPathParser::current_token() {
    if (m_token_consumed) {
        m_current_token  = m_lexer.next_token();
        m_token_consumed = false;
    }
    return m_current_token;
}

void XPathParser::advance() {
    m_token_consumed = true;
}

bool XPathParser::match(XPathTokenType type) const {
    return const_cast<XPathParser*>(this)->current_token().type == type;
}

bool XPathParser::match_any(const std::vector<XPathTokenType>& types) const {
    auto current_type = const_cast<XPathParser*>(this)->current_token().type;
    return std::ranges::find(types, current_type) != types.end();
}

void XPathParser::consume(XPathTokenType type) {
    if (!match(type)) {
        throw_parse_error("Expected " + std::to_string(static_cast<int>(type)) + " but got " + std::to_string(static_cast<int>(current_token().type)));
    }
    advance();
}

bool XPathParser::consume_if(XPathTokenType type) {
    if (match(type)) {
        advance();
        return true;
    }
    return false;
}

//==============================================================================
// Expression Parsing (by precedence, low to high)
//==============================================================================

std::unique_ptr<XPathExpression> XPathParser::parse_or_expression() {
    auto left = parse_and_expression();

    while (match(XPathTokenType::OR)) {
        advance();
        auto right = parse_and_expression();
        left       = std::make_unique<XPathBinaryExpression>(std::move(left), XPathBinaryExpression::Operator::OR, std::move(right));
    }

    return left;
}

std::unique_ptr<XPathExpression> XPathParser::parse_and_expression() {
    auto left = parse_equality_expression();

    while (match(XPathTokenType::AND)) {
        advance();
        auto right = parse_equality_expression();
        left       = std::make_unique<XPathBinaryExpression>(std::move(left), XPathBinaryExpression::Operator::AND, std::move(right));
    }

    return left;
}

std::unique_ptr<XPathExpression> XPathParser::parse_equality_expression() {
    auto left = parse_relational_expression();

    while (match_any({XPathTokenType::EQUAL, XPathTokenType::NOT_EQUAL})) {
        auto op_type = current_token().type;
        advance();
        auto right = parse_relational_expression();

        auto op = (op_type == XPathTokenType::EQUAL) ? XPathBinaryExpression::Operator::EQUAL : XPathBinaryExpression::Operator::NOT_EQUAL;

        left = std::make_unique<XPathBinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

std::unique_ptr<XPathExpression> XPathParser::parse_relational_expression() {
    auto left = parse_additive_expression();

    while (match_any({XPathTokenType::LESS, XPathTokenType::LESS_EQUAL, XPathTokenType::GREATER, XPathTokenType::GREATER_EQUAL})) {
        auto op_type = current_token().type;
        advance();
        auto right = parse_additive_expression();

        XPathBinaryExpression::Operator op;
        switch (op_type) {
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
                throw_parse_error("Invalid relational operator");
        }

        left = std::make_unique<XPathBinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

std::unique_ptr<XPathExpression> XPathParser::parse_additive_expression() {
    auto left = parse_multiplicative_expression();

    while (match_any({XPathTokenType::PLUS, XPathTokenType::MINUS})) {
        auto op_type = current_token().type;
        advance();
        auto right = parse_multiplicative_expression();

        auto op = (op_type == XPathTokenType::PLUS) ? XPathBinaryExpression::Operator::PLUS : XPathBinaryExpression::Operator::MINUS;

        left = std::make_unique<XPathBinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

std::unique_ptr<XPathExpression> XPathParser::parse_multiplicative_expression() {
    auto left = parse_unary_expression();

    while (match_any({XPathTokenType::MULTIPLY, XPathTokenType::DIVIDE, XPathTokenType::MODULO})) {
        auto op_type = current_token().type;
        advance();
        auto right = parse_unary_expression();

        XPathBinaryExpression::Operator op;
        switch (op_type) {
            case XPathTokenType::MULTIPLY:
                op = XPathBinaryExpression::Operator::MULTIPLY;
                break;
            case XPathTokenType::DIVIDE:
                op = XPathBinaryExpression::Operator::DIVIDE;
                break;
            case XPathTokenType::MODULO:
                op = XPathBinaryExpression::Operator::MODULO;
                break;
            default:
                throw_parse_error("Invalid multiplicative operator");
        }

        left = std::make_unique<XPathBinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

std::unique_ptr<XPathExpression> XPathParser::parse_unary_expression() {
    if (match(XPathTokenType::MINUS)) {
        advance();
        auto operand = parse_unary_expression();
        return std::make_unique<XPathUnaryExpression>(XPathUnaryExpression::Operator::NEGATE, std::move(operand));
    }

    return parse_union_expression();
}

std::unique_ptr<XPathExpression> XPathParser::parse_union_expression() {
    auto left = parse_path_expression();

    while (match(XPathTokenType::UNION)) {
        advance();
        auto right = parse_path_expression();
        left       = std::make_unique<XPathBinaryExpression>(std::move(left), XPathBinaryExpression::Operator::UNION, std::move(right));
    }

    return left;
}

std::unique_ptr<XPathExpression> XPathParser::parse_path_expression() {
    // 首先尝试解析过滤表达式
    auto expr = parse_filter_expression();

    // 检查是否有路径步骤跟随
    if (match_any({XPathTokenType::SLASH, XPathTokenType::DOUBLE_SLASH})) {
        auto separator_type = current_token().type;
        advance();

        // 解析相对位置路径
        auto relative_path = parse_relative_location_path();

        // 创建链式路径表达式
        auto separator = (separator_type == XPathTokenType::SLASH) ? XPathChainedPath::Separator::SLASH : XPathChainedPath::Separator::DOUBLE_SLASH;

        return std::make_unique<XPathChainedPath>(std::move(expr), separator, std::move(relative_path));
    }

    return expr;
}

std::unique_ptr<XPathExpression> XPathParser::parse_filter_expression() {
    auto primary = parse_primary_expression();

    // 检查是否有谓词
    if (match(XPathTokenType::LBRACKET)) {
        auto predicates = parse_predicates();
        return std::make_unique<XPathFilterExpression>(std::move(primary), std::move(predicates));
    }

    return primary;
}

std::unique_ptr<XPathExpression> XPathParser::parse_primary_expression() {
    // 变量引用
    if (match(XPathTokenType::VARIABLE)) {
        auto full_name = current_token().value;  // 包含 $ 前缀的完整名称
        auto name      = full_name.substr(1);    // 去掉 $ 前缀
        advance();
        return std::make_unique<XPathVariableReference>(name);
    }

    // 数字字面量
    if (match(XPathTokenType::NUMBER_LITERAL)) {
        auto value = parse_number_literal();
        return std::make_unique<XPathNumber>(value);
    }

    // 字符串字面量
    if (match(XPathTokenType::STRING_LITERAL)) {
        auto value = parse_string_literal();
        return std::make_unique<XPathLiteral>(value);
    }

    // 括号表达式
    if (match(XPathTokenType::LPAREN)) {
        advance();
        auto expr = parse_or_expression();
        consume(XPathTokenType::RPAREN);
        return expr;
    }

    // 函数调用 - 直接检查 FUNCTION_NAME token
    if (match(XPathTokenType::FUNCTION_NAME)) {
        return parse_function_call();
    }

    // 节点类型测试作为独立表达式 - 直接检查节点类型 token
    if (match_any({XPathTokenType::NODE_TYPE_COMMENT, XPathTokenType::NODE_TYPE_TEXT, 
                   XPathTokenType::NODE_TYPE_PROCESSING_INSTRUCTION, XPathTokenType::NODE_TYPE_NODE})) {
        // 创建一个包含单个步骤的相对位置路径
        auto node_test  = parse_node_test();
        auto predicates = std::vector<std::unique_ptr<XPathPredicate>>();
        auto step       = std::make_unique<XPathStep>(XPathAxis::CHILD, std::move(node_test), std::move(predicates));

        std::vector<std::unique_ptr<XPathStep>> steps;
        steps.push_back(std::move(step));

        return std::make_unique<XPathLocationPath>(std::move(steps), false);
    }

    // IDENTIFIER 后跟 LPAREN 的函数调用（备用检查）
    if (match(XPathTokenType::IDENTIFIER) && m_lexer.peek_token().type == XPathTokenType::LPAREN) {
        return parse_function_call();
    }

    // 位置路径表达式
    if (is_location_path_start()) {
        return parse_location_path();
    }

    throw_parse_error("Expected primary expression");
}

//==============================================================================
// Location Path Parsing
//==============================================================================

bool XPathParser::is_location_path_start() {
    return match_any({
               XPathTokenType::SLASH,         // 绝对路径 /
               XPathTokenType::DOUBLE_SLASH,  // 递归下降 //
               XPathTokenType::DOT,           // 当前节点 .
               XPathTokenType::DOUBLE_DOT,    // 父节点 ..
               XPathTokenType::AT,            // 属性 @
               XPathTokenType::STAR,          // 通配符 *
               XPathTokenType::IDENTIFIER     // 元素名
           })
           || is_axis_specifier();  // 或者轴标识符
}

std::unique_ptr<XPathLocationPath> XPathParser::parse_location_path() {
    bool is_absolute = false;

    // 检查是否是绝对路径
    if (match(XPathTokenType::SLASH)) {
        is_absolute = true;
        advance();

        // 如果只有一个 /，后面没有步骤，返回根节点路径
        if (!is_step_start()) {
            return std::make_unique<XPathLocationPath>(std::vector<std::unique_ptr<XPathStep>>(), true);
        }
    } else if (match(XPathTokenType::DOUBLE_SLASH)) {
        is_absolute = true;
        advance();

        // // 等价于 /descendant-or-self::node()/
        auto descendant_step = std::make_unique<XPathStep>(XPathAxis::DESCENDANT_OR_SELF, std::make_unique<XPathNodeTypeTest>(XPathNodeTypeTest::NodeType::NODE), std::vector<std::unique_ptr<XPathPredicate>>());

        std::vector<std::unique_ptr<XPathStep>> steps;
        steps.push_back(std::move(descendant_step));

        // 解析后续的相对路径步骤
        if (is_step_start()) {
            // 修复：直接解析单个步骤，然后继续解析后续步骤
            steps.push_back(parse_step());

            // 继续解析可能的后续步骤
            while (match_any({XPathTokenType::SLASH, XPathTokenType::DOUBLE_SLASH})) {
                auto separator = current_token().type;
                advance();

                if (separator == XPathTokenType::DOUBLE_SLASH) {
                    // 添加 descendant-or-self::node() 步骤
                    auto desc_step = std::make_unique<XPathStep>(XPathAxis::DESCENDANT_OR_SELF, std::make_unique<XPathNodeTypeTest>(XPathNodeTypeTest::NodeType::NODE), std::vector<std::unique_ptr<XPathPredicate>>());
                    steps.push_back(std::move(desc_step));
                }

                if (is_step_start()) {
                    steps.push_back(parse_step());
                }
            }
        }

        return std::make_unique<XPathLocationPath>(std::move(steps), true);
    }

    // 解析相对位置路径
    auto steps = parse_relative_location_path_steps();
    return std::make_unique<XPathLocationPath>(std::move(steps), is_absolute);
}

std::unique_ptr<XPathLocationPath> XPathParser::parse_absolute_location_path() {
    if (match(XPathTokenType::SLASH)) {
        advance();

        // 如果只有一个 /，后面没有步骤，返回根节点路径
        if (!is_step_start()) {
            return std::make_unique<XPathLocationPath>(std::vector<std::unique_ptr<XPathStep>>(), true);
        }

        // 解析后续的相对路径步骤
        auto steps = parse_relative_location_path_steps();
        return std::make_unique<XPathLocationPath>(std::move(steps), true);
    } else if (match(XPathTokenType::DOUBLE_SLASH)) {
        advance();

        // // 等价于 /descendant-or-self::node()/
        auto descendant_step = std::make_unique<XPathStep>(XPathAxis::DESCENDANT_OR_SELF, std::make_unique<XPathNodeTypeTest>(XPathNodeTypeTest::NodeType::NODE), std::vector<std::unique_ptr<XPathPredicate>>());

        std::vector<std::unique_ptr<XPathStep>> steps;
        steps.push_back(std::move(descendant_step));

        // 解析后续的相对路径步骤
        if (is_step_start()) {
            auto relative_steps = parse_relative_location_path_steps();
            for (auto& step : relative_steps) {
                steps.push_back(std::move(step));
            }
        }

        return std::make_unique<XPathLocationPath>(std::move(steps), true);
    }

    throw_parse_error("Expected '/' or '//' for absolute location path");
}

std::unique_ptr<XPathLocationPath> XPathParser::parse_relative_location_path() {
    auto steps = parse_relative_location_path_steps();
    return std::make_unique<XPathLocationPath>(std::move(steps), false);
}

std::vector<std::unique_ptr<XPathStep>> XPathParser::parse_relative_location_path_steps() {
    std::vector<std::unique_ptr<XPathStep>> steps;

    if (is_step_start()) {
        steps.push_back(parse_step());

        while (match_any({XPathTokenType::SLASH, XPathTokenType::DOUBLE_SLASH})) {
            auto separator = current_token().type;
            advance();

            if (separator == XPathTokenType::DOUBLE_SLASH) {
                // 添加 descendant-or-self::node() 步骤
                auto descendant_step = std::make_unique<XPathStep>(XPathAxis::DESCENDANT_OR_SELF, std::make_unique<XPathNodeTypeTest>(XPathNodeTypeTest::NodeType::NODE), std::vector<std::unique_ptr<XPathPredicate>>());
                steps.push_back(std::move(descendant_step));
            }

            if (is_step_start()) {
                steps.push_back(parse_step());
            }
        }
    }

    return steps;
}

bool XPathParser::is_step_start() {
    return match_any({
               XPathTokenType::DOT,         // 当前节点 .
               XPathTokenType::DOUBLE_DOT,  // 父节点 ..
               XPathTokenType::AT,          // 属性 @
               XPathTokenType::STAR,        // 通配符 *
               XPathTokenType::IDENTIFIER,  // 元素名
               XPathTokenType::DIVIDE       // div 元素名（被误识别为除法操作符）
           })
           || is_axis_specifier() || is_node_type_test();
}

std::unique_ptr<XPathStep> XPathParser::parse_step() {
    XPathAxis axis = XPathAxis::CHILD;  // 默认轴

    // 处理缩写语法
    if (match(XPathTokenType::DOT)) {
        // . 等价于 self::node()
        advance();
        auto node_test  = std::make_unique<XPathNodeTypeTest>(XPathNodeTypeTest::NodeType::NODE);
        auto predicates = parse_predicates();
        return std::make_unique<XPathStep>(XPathAxis::SELF, std::move(node_test), std::move(predicates));
    }

    if (match(XPathTokenType::DOUBLE_DOT)) {
        // .. 等价于 parent::node()
        advance();
        auto node_test  = std::make_unique<XPathNodeTypeTest>(XPathNodeTypeTest::NodeType::NODE);
        auto predicates = parse_predicates();
        return std::make_unique<XPathStep>(XPathAxis::PARENT, std::move(node_test), std::move(predicates));
    }

    if (match(XPathTokenType::AT)) {
        // @ 等价于 attribute::
        advance();
        axis = XPathAxis::ATTRIBUTE;
    } else if (is_axis_specifier()) {
        // 显式轴标识符
        axis = parse_axis_specifier();
        // 对于轴 token，不需要消费 DOUBLE_COLON，因为它已经包含在 token 中
        // 对于 IDENTIFIER 形式的轴标识符，仍需要消费 DOUBLE_COLON
        if (match(XPathTokenType::DOUBLE_COLON)) {
            consume(XPathTokenType::DOUBLE_COLON);
        }
    }

    // 解析节点测试
    auto node_test = parse_node_test();

    // 解析谓词
    auto predicates = parse_predicates();

    return std::make_unique<XPathStep>(axis, std::move(node_test), std::move(predicates));
}

std::unique_ptr<XPathStep> XPathParser::parse_abbreviated_step() {
    if (match(XPathTokenType::DOT)) {
        // . 等价于 self::node()
        advance();
        auto node_test  = std::make_unique<XPathNodeTypeTest>(XPathNodeTypeTest::NodeType::NODE);
        auto predicates = parse_predicates();
        return std::make_unique<XPathStep>(XPathAxis::SELF, std::move(node_test), std::move(predicates));
    }

    if (match(XPathTokenType::DOUBLE_DOT)) {
        // .. 等价于 parent::node()
        advance();
        auto node_test  = std::make_unique<XPathNodeTypeTest>(XPathNodeTypeTest::NodeType::NODE);
        auto predicates = parse_predicates();
        return std::make_unique<XPathStep>(XPathAxis::PARENT, std::move(node_test), std::move(predicates));
    }

    throw_parse_error("Expected '.' or '..' for abbreviated step");
}

//==============================================================================
// Node Test and Predicate Parsing
//==============================================================================

std::unique_ptr<XPathNodeTest> XPathParser::parse_node_test() {
    // 检查是否是节点类型测试
    if (is_node_type_test()) {
        // 处理节点类型 token
        if (match(XPathTokenType::NODE_TYPE_COMMENT)) {
            advance();
            consume(XPathTokenType::LPAREN);
            consume(XPathTokenType::RPAREN);
            return std::make_unique<XPathNodeTypeTest>(XPathNodeTypeTest::NodeType::COMMENT);
        }

        if (match(XPathTokenType::NODE_TYPE_TEXT)) {
            advance();
            consume(XPathTokenType::LPAREN);
            consume(XPathTokenType::RPAREN);
            return std::make_unique<XPathNodeTypeTest>(XPathNodeTypeTest::NodeType::TEXT);
        }

        if (match(XPathTokenType::NODE_TYPE_NODE)) {
            advance();
            consume(XPathTokenType::LPAREN);
            consume(XPathTokenType::RPAREN);
            return std::make_unique<XPathNodeTypeTest>(XPathNodeTypeTest::NodeType::NODE);
        }

        if (match(XPathTokenType::NODE_TYPE_PROCESSING_INSTRUCTION)) {
            advance();
            consume(XPathTokenType::LPAREN);

            // 处理 processing-instruction 的可选参数
            std::string target;
            if (match(XPathTokenType::STRING_LITERAL)) {
                target = parse_string_literal();
            }

            consume(XPathTokenType::RPAREN);
            return std::make_unique<XPathPITest>(target);
        }

        // 处理 IDENTIFIER 形式的节点类型测试
        return parse_node_type_test();
    }

    // 处理通配符
    if (match(XPathTokenType::STAR)) {
        advance();
        return std::make_unique<XPathNameTest>("*");
    }

    // 处理名称测试
    if (match(XPathTokenType::IDENTIFIER)) {
        std::string name = current_token().value;
        advance();

        // 检查是否是 prefix:localname 形式
        if (match(XPathTokenType::COLON)) {
            advance();
            if (match(XPathTokenType::IDENTIFIER)) {
                std::string local_name = current_token().value;
                advance();
                return std::make_unique<XPathNameTest>(name + ":" + local_name);
            } else if (match(XPathTokenType::STAR)) {
                advance();
                return std::make_unique<XPathNameTest>(name + ":*");
            } else {
                throw_parse_error("Expected identifier or '*' after ':'");
            }
        }

        return std::make_unique<XPathNameTest>(name);
    }

    // 处理 div 元素名（被词法分析器误识别为 DIVIDE token）
    if (match(XPathTokenType::DIVIDE)) {
        std::string name = current_token().value;
        advance();
        return std::make_unique<XPathNameTest>(name);
    }

    throw_parse_error("Expected node test");
}

bool XPathParser::is_operator() const {
    return match_any({XPathTokenType::OR,
                      XPathTokenType::AND,
                      XPathTokenType::EQUAL,
                      XPathTokenType::NOT_EQUAL,
                      XPathTokenType::LESS,
                      XPathTokenType::LESS_EQUAL,
                      XPathTokenType::GREATER,
                      XPathTokenType::GREATER_EQUAL,
                      XPathTokenType::PLUS,
                      XPathTokenType::MINUS,
                      XPathTokenType::MULTIPLY,
                      XPathTokenType::DIVIDE,
                      XPathTokenType::MODULO,
                      XPathTokenType::UNION});
}

XPathBinaryExpression::Operator XPathParser::token_to_binary_operator(XPathTokenType type) const {
    switch (type) {
        case XPathTokenType::OR:
            return XPathBinaryExpression::Operator::OR;
        case XPathTokenType::AND:
            return XPathBinaryExpression::Operator::AND;
        case XPathTokenType::EQUAL:
            return XPathBinaryExpression::Operator::EQUAL;
        case XPathTokenType::NOT_EQUAL:
            return XPathBinaryExpression::Operator::NOT_EQUAL;
        case XPathTokenType::LESS:
            return XPathBinaryExpression::Operator::LESS;
        case XPathTokenType::LESS_EQUAL:
            return XPathBinaryExpression::Operator::LESS_EQUAL;
        case XPathTokenType::GREATER:
            return XPathBinaryExpression::Operator::GREATER;
        case XPathTokenType::GREATER_EQUAL:
            return XPathBinaryExpression::Operator::GREATER_EQUAL;
        case XPathTokenType::PLUS:
            return XPathBinaryExpression::Operator::PLUS;
        case XPathTokenType::MINUS:
            return XPathBinaryExpression::Operator::MINUS;
        case XPathTokenType::MULTIPLY:
            return XPathBinaryExpression::Operator::MULTIPLY;
        case XPathTokenType::DIVIDE:
            return XPathBinaryExpression::Operator::DIVIDE;
        case XPathTokenType::MODULO:
            return XPathBinaryExpression::Operator::MODULO;
        case XPathTokenType::UNION:
            return XPathBinaryExpression::Operator::UNION;
        default:
            throw_parse_error("Unknown binary operator");
    }
}

std::string XPathParser::parse_string_literal() {
    if (!match(XPathTokenType::STRING_LITERAL)) {
        throw_parse_error("Expected string literal");
    }

    auto value = current_token().value;
    advance();
    return value;
}

double XPathParser::parse_number_literal() {
    if (!match(XPathTokenType::NUMBER_LITERAL)) {
        throw_parse_error("Expected number literal");
    }

    std::string number_str = current_token().value;
    advance();

    try {
        return std::stod(number_str);
    } catch (const std::exception&) {
        throw_parse_error("Invalid number format: " + number_str);
    }
}

std::string XPathParser::create_error_message(const std::string& expected) const {
    std::ostringstream oss;
    oss << "Expected " << expected << " but got ";

    auto& token = const_cast<XPathParser*>(this)->current_token();
    if (token.type == XPathTokenType::END_OF_FILE) {
        oss << "end of input";
    } else {
        oss << "'" << token.value << "'";
    }

    return oss.str();
}

void XPathParser::throw_parse_error(const std::string& message) const {
    throw XPathParseError(message);
}

std::vector<std::unique_ptr<XPathPredicate>> XPathParser::parse_predicates() {
    std::vector<std::unique_ptr<XPathPredicate>> predicates;

    while (match(XPathTokenType::LBRACKET)) {
        predicates.push_back(parse_predicate());
    }

    return predicates;
}

std::unique_ptr<XPathPredicate> XPathParser::parse_predicate() {
    consume(XPathTokenType::LBRACKET);
    auto expression = parse_or_expression();
    consume(XPathTokenType::RBRACKET);

    return std::make_unique<XPathPredicate>(std::move(expression));
}

std::unique_ptr<XPathFunctionCall> XPathParser::parse_function_call() {
    std::string function_name;
    
    // 处理 FUNCTION_NAME token
    if (match(XPathTokenType::FUNCTION_NAME)) {
        function_name = current_token().value;
        advance();
    }
    // 处理 IDENTIFIER token（备用）
    else if (match(XPathTokenType::IDENTIFIER)) {
        function_name = current_token().value;
        advance();
    }
    else {
        throw_parse_error("Expected function name");
    }

    consume(XPathTokenType::LPAREN);

    auto arguments = parse_argument_list();

    consume(XPathTokenType::RPAREN);

    return std::make_unique<XPathFunctionCall>(function_name, std::move(arguments));
}

std::vector<std::unique_ptr<XPathExpression>> XPathParser::parse_argument_list() {
    std::vector<std::unique_ptr<XPathExpression>> arguments;

    // 空参数列表
    if (match(XPathTokenType::RPAREN)) {
        return arguments;
    }

    // 解析第一个参数
    arguments.push_back(parse_or_expression());

    // 解析后续参数
    while (match(XPathTokenType::COMMA)) {
        advance();
        arguments.push_back(parse_or_expression());
    }

    return arguments;
}

bool XPathParser::is_axis_specifier() {
    // 检查轴 token
    if (match_any({XPathTokenType::AXIS_ANCESTOR,
                   XPathTokenType::AXIS_ANCESTOR_OR_SELF,
                   XPathTokenType::AXIS_ATTRIBUTE,
                   XPathTokenType::AXIS_CHILD,
                   XPathTokenType::AXIS_DESCENDANT,
                   XPathTokenType::AXIS_DESCENDANT_OR_SELF,
                   XPathTokenType::AXIS_FOLLOWING,
                   XPathTokenType::AXIS_FOLLOWING_SIBLING,
                   XPathTokenType::AXIS_NAMESPACE,
                   XPathTokenType::AXIS_PARENT,
                   XPathTokenType::AXIS_PRECEDING,
                   XPathTokenType::AXIS_PRECEDING_SIBLING,
                   XPathTokenType::AXIS_SELF})) {
        return true;
    }

    // 保持原有的 IDENTIFIER 检查逻辑（用于处理不完整的轴标识符）
    if (!match(XPathTokenType::IDENTIFIER)) {
        return false;
    }

    const std::string& value = current_token().value;
    return value == "ancestor" || value == "ancestor-or-self" || value == "attribute" || value == "child" || value == "descendant" || value == "descendant-or-self" || value == "following" || value == "following-sibling" || value == "namespace" || value == "parent"
           || value == "preceding" || value == "preceding-sibling" || value == "self";
}

XPathAxis XPathParser::parse_axis_specifier() {
    // 处理轴 token
    if (match(XPathTokenType::AXIS_ANCESTOR)) {
        advance();
        return XPathAxis::ANCESTOR;
    }
    if (match(XPathTokenType::AXIS_ANCESTOR_OR_SELF)) {
        advance();
        return XPathAxis::ANCESTOR_OR_SELF;
    }
    if (match(XPathTokenType::AXIS_ATTRIBUTE)) {
        advance();
        return XPathAxis::ATTRIBUTE;
    }
    if (match(XPathTokenType::AXIS_CHILD)) {
        advance();
        return XPathAxis::CHILD;
    }
    if (match(XPathTokenType::AXIS_DESCENDANT)) {
        advance();
        return XPathAxis::DESCENDANT;
    }
    if (match(XPathTokenType::AXIS_DESCENDANT_OR_SELF)) {
        advance();
        return XPathAxis::DESCENDANT_OR_SELF;
    }
    if (match(XPathTokenType::AXIS_FOLLOWING)) {
        advance();
        return XPathAxis::FOLLOWING;
    }
    if (match(XPathTokenType::AXIS_FOLLOWING_SIBLING)) {
        advance();
        return XPathAxis::FOLLOWING_SIBLING;
    }
    if (match(XPathTokenType::AXIS_NAMESPACE)) {
        advance();
        return XPathAxis::NAMESPACE;
    }
    if (match(XPathTokenType::AXIS_PARENT)) {
        advance();
        return XPathAxis::PARENT;
    }
    if (match(XPathTokenType::AXIS_PRECEDING)) {
        advance();
        return XPathAxis::PRECEDING;
    }
    if (match(XPathTokenType::AXIS_PRECEDING_SIBLING)) {
        advance();
        return XPathAxis::PRECEDING_SIBLING;
    }
    if (match(XPathTokenType::AXIS_SELF)) {
        advance();
        return XPathAxis::SELF;
    }

    // 保持原有的 IDENTIFIER 处理逻辑
    if (!match(XPathTokenType::IDENTIFIER)) {
        throw_parse_error("Expected axis specifier");
    }

    const std::string& axis_name = current_token().value;
    advance();

    if (axis_name == "ancestor")
        return XPathAxis::ANCESTOR;
    if (axis_name == "ancestor-or-self")
        return XPathAxis::ANCESTOR_OR_SELF;
    if (axis_name == "attribute")
        return XPathAxis::ATTRIBUTE;
    if (axis_name == "child")
        return XPathAxis::CHILD;
    if (axis_name == "descendant")
        return XPathAxis::DESCENDANT;
    if (axis_name == "descendant-or-self")
        return XPathAxis::DESCENDANT_OR_SELF;
    if (axis_name == "following")
        return XPathAxis::FOLLOWING;
    if (axis_name == "following-sibling")
        return XPathAxis::FOLLOWING_SIBLING;
    if (axis_name == "namespace")
        return XPathAxis::NAMESPACE;
    if (axis_name == "parent")
        return XPathAxis::PARENT;
    if (axis_name == "preceding")
        return XPathAxis::PRECEDING;
    if (axis_name == "preceding-sibling")
        return XPathAxis::PRECEDING_SIBLING;
    if (axis_name == "self")
        return XPathAxis::SELF;

    throw_parse_error("Unknown axis specifier: " + axis_name);
}

bool XPathParser::is_node_type_test() {
    // 检查节点类型 token
    if (match_any({XPathTokenType::NODE_TYPE_COMMENT, XPathTokenType::NODE_TYPE_TEXT, XPathTokenType::NODE_TYPE_PROCESSING_INSTRUCTION, XPathTokenType::NODE_TYPE_NODE})) {
        return true;
    }

    // 保持原有的 IDENTIFIER 检查逻辑（用于处理不完整的节点类型测试）
    if (!match(XPathTokenType::IDENTIFIER)) {
        return false;
    }

    const std::string& value = current_token().value;
    return value == "comment" || value == "text" || value == "processing-instruction" || value == "node";
}

std::unique_ptr<XPathNodeTest> XPathParser::parse_node_type_test() {
    XPathNodeTypeTest::NodeType node_type;
    
    // 处理节点类型 token
    if (match(XPathTokenType::NODE_TYPE_COMMENT)) {
        node_type = XPathNodeTypeTest::NodeType::COMMENT;
        advance();
    }
    else if (match(XPathTokenType::NODE_TYPE_TEXT)) {
        node_type = XPathNodeTypeTest::NodeType::TEXT;
        advance();
    }
    else if (match(XPathTokenType::NODE_TYPE_NODE)) {
        node_type = XPathNodeTypeTest::NodeType::NODE;
        advance();
    }
    else if (match(XPathTokenType::NODE_TYPE_PROCESSING_INSTRUCTION)) {
        // 处理 processing-instruction，这里简化为 COMMENT 类型
        node_type = XPathNodeTypeTest::NodeType::COMMENT;
        advance();
    }
    // 备用：处理 IDENTIFIER token
    else if (match(XPathTokenType::IDENTIFIER)) {
        const std::string& value = current_token().value;
        if (value == "comment") {
            node_type = XPathNodeTypeTest::NodeType::COMMENT;
        } else if (value == "text") {
            node_type = XPathNodeTypeTest::NodeType::TEXT;
        } else if (value == "node") {
            node_type = XPathNodeTypeTest::NodeType::NODE;
        } else {
            throw_parse_error("Unknown node type: " + value);
        }
        advance();
    }
    else {
        throw_parse_error("Expected node type test");
    }

    consume(XPathTokenType::LPAREN);
    consume(XPathTokenType::RPAREN);

    return std::make_unique<XPathNodeTypeTest>(node_type);
}
}  // namespace hps