#include "hps/query/xpath/xpath_ast.hpp"

#include <sstream>
#include <string>

namespace hps {

//==============================================================================
// 基本表达式类实现 (Basic Expression Classes Implementation)
//==============================================================================

std::string XPathLiteral::to_string() const {
    bool has_single_quote = (m_value.find('\'') != std::string::npos);
    bool has_double_quote = (m_value.find('"') != std::string::npos);

    // Case 1: 字符串不包含双引号，优先使用双引号包围
    if (!has_double_quote) {
        return "\"" + m_value + "\"";
    }

    // Case 2: 字符串不包含单引号（但肯定包含双引号），使用单引号包围
    if (!has_single_quote) {
        return "'" + m_value + "'";
    }

    // Case 3: 字符串同时包含单引号和双引号，必须使用 concat() 函数
    // 我们选择将字符串按单引号分割
    std::ostringstream oss;
    oss << "concat(";
    std::string::size_type start = 0;
    std::string::size_type pos;
    bool                   first_part = true;

    // 遍历字符串，找到所有的单引号
    while ((pos = m_value.find('\'', start)) != std::string::npos) {
        if (!first_part) {
            oss << ", ";
        }

        // 1. 添加从上一个单引号到当前单引号之间的子串（用单引号包围）
        oss << "'" << m_value.substr(start, pos - start) << "'";

        // 2. 添加表示单引号本身的字面量（必须用双引号包围）
        oss << ", '\"'";

        start      = pos + 1;
        first_part = false;
    }

    // 添加最后一个单引号之后剩余的子串
    if (start < m_value.length()) {
        if (!first_part) {
            oss << ", ";
        }
        oss << "'" << m_value.substr(start) << "'";
    }

    oss << ")";
    return oss.str();
}

std::string XPathNumber::to_string() const {
    std::ostringstream oss;
    oss << m_value;
    return oss.str();
}

std::string XPathVariableReference::to_string() const {
    return "$" + m_name;
}

//==============================================================================
// 运算符表达式类实现 (Operator Expression Classes Implementation)
//==============================================================================

std::string XPathUnaryExpression::to_string() const {
    std::ostringstream oss;
    switch (m_operator) {
        case Operator::NEGATE:
            oss << "-";
            break;
    }
    oss << m_operand->to_string();
    return oss.str();
}

std::string XPathBinaryExpression::to_string() const {
    std::ostringstream oss;
    oss << "(" << m_left->to_string() << " ";
    switch (m_operator) {
        case Operator::OR:
            oss << "or";
            break;
        case Operator::AND:
            oss << "and";
            break;
        case Operator::EQUAL:
            oss << "=";
            break;
        case Operator::NOT_EQUAL:
            oss << "!=";
            break;
        case Operator::LESS:
            oss << "<";
            break;
        case Operator::LESS_EQUAL:
            oss << "<=";
            break;
        case Operator::GREATER:
            oss << ">";
            break;
        case Operator::GREATER_EQUAL:
            oss << ">=";
            break;
        case Operator::PLUS:
            oss << "+";
            break;
        case Operator::MINUS:
            oss << "-";
            break;
        case Operator::MULTIPLY:
            oss << "*";
            break;
        case Operator::DIVIDE:
            oss << "div";
            break;
        case Operator::MODULO:
            oss << "mod";
            break;
        case Operator::UNION:
            oss << "|";
            break;
    }
    oss << " " << m_right->to_string() << ")";
    return oss.str();
}

std::string XPathFunctionCall::to_string() const {
    std::ostringstream oss;
    oss << m_name << "(";
    for (size_t i = 0; i < m_arguments.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << m_arguments[i]->to_string();
    }
    oss << ")";
    return oss.str();
}

//==============================================================================
// 节点测试类实现 (Node Test Classes Implementation)
//==============================================================================

std::string XPathNameTest::to_string() const {
    if (!m_prefix.empty()) {
        return m_prefix + ":" + m_local_name;
    }
    return m_local_name;
}

std::string XPathNodeTypeTest::to_string() const {
    switch (m_type) {
        case NodeType::COMMENT:
            return "comment()";
        case NodeType::TEXT:
            return "text()";
        case NodeType::NODE:
            return "node()";
    }
    return "";  // Should not happen
}

// NEW: XPathPITest implementation
std::string XPathPITest::to_string() const {
    if (m_target.empty()) {
        return "processing-instruction()";
    }

    bool has_single_quote = (m_target.find('\'') != std::string::npos);
    bool has_double_quote = (m_target.find('"') != std::string::npos);

    // Case 1: 目标字符串不包含单引号，使用单引号包围
    if (!has_single_quote) {
        return "processing-instruction('" + m_target + "')";
    }

    // Case 2: 目标字符串不包含双引号（但肯定包含单引号），使用双引号包围
    if (!has_double_quote) {
        return "processing-instruction(\"" + m_target + "\")";
    }

    // Case 3: 目标字符串同时包含单引号和双引号
    // 这种情况在 XPath 1.0 中无法表示为一个字面量，因此是不合法的。
    // 我们应该抛出异常而不是生成一个无效的表达式。
    throw std::logic_error(
        "Cannot create a valid processing-instruction() test in XPath 1.0 "
        "for a target literal that contains both single and double quotes.");
}

//==============================================================================
// 路径相关类实现 (Path-related Classes Implementation)
//==============================================================================

std::string XPathPredicate::to_string() const {
    return m_expression->to_string();
}

std::string XPathStep::to_string() const {
    std::ostringstream oss;
    switch (m_axis) {
        case XPathAxis::ANCESTOR:
            oss << "ancestor::";
            break;
        case XPathAxis::ANCESTOR_OR_SELF:
            oss << "ancestor-or-self::";
            break;
        case XPathAxis::ATTRIBUTE:
            oss << "@";
            break;
        case XPathAxis::DESCENDANT:
            oss << "descendant::";
            break;
        case XPathAxis::DESCENDANT_OR_SELF:
            oss << "descendant-or-self::";
            break;
        case XPathAxis::FOLLOWING:
            oss << "following::";
            break;
        case XPathAxis::FOLLOWING_SIBLING:
            oss << "following-sibling::";
            break;
        case XPathAxis::NAMESPACE:
            oss << "namespace::";
            break;
        case XPathAxis::PARENT:
            oss << "parent::";
            break;
        case XPathAxis::PRECEDING:
            oss << "preceding::";
            break;
        case XPathAxis::PRECEDING_SIBLING:
            oss << "preceding-sibling::";
            break;
        case XPathAxis::SELF:
            oss << "self::";
            break;
        case XPathAxis::CHILD:
            // child 轴是默认轴，通常省略
            break;
    }

    oss << m_node_test->to_string();

    for (const auto& predicate : m_predicates) {
        oss << "[" << predicate->to_string() << "]";
    }
    return oss.str();
}

// IMPROVED: Simplified XPathLocationPath::to_string implementation
std::string XPathLocationPath::to_string() const {
    std::ostringstream oss;
    if (m_is_absolute) {
        oss << "/";
    }
    for (size_t i = 0; i < m_steps.size(); ++i) {
        if (i > 0) {
            oss << "/";
        }
        oss << m_steps[i]->to_string();
    }
    return oss.str();
}

std::string XPathFilterExpression::to_string() const {
    std::ostringstream oss;

    // 仅当主表达式是二元/一元表达式时加括号，以保持优先级
    const bool needs_parentheses = dynamic_cast<const XPathBinaryExpression*>(m_primary_expr.get()) != nullptr || dynamic_cast<const XPathUnaryExpression*>(m_primary_expr.get()) != nullptr;

    if (needs_parentheses) {
        oss << "(";
    }
    oss << m_primary_expr->to_string();
    if (needs_parentheses) {
        oss << ")";
    }

    for (const auto& predicate : m_predicates) {
        oss << "[" << predicate->to_string() << "]";
    }
    return oss.str();
}

// NEW: XPathChainedPath implementation
std::string XPathChainedPath::to_string() const {
    std::ostringstream oss;

    // 过滤表达式通常需要括号来确保求值顺序
    oss << "(" << m_filter_expr->to_string() << ")";

    if (m_separator == Separator::DOUBLE_SLASH) {
        oss << "//";
    } else {
        oss << "/";
    }

    oss << m_relative_path->to_string();
    return oss.str();
}

}  // namespace hps