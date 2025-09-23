#include "hps/query/xpath/xpath_ast.hpp"

#include <sstream>
#include <string>

namespace hps {

//==============================================================================
// 基本表达式类实现 (Basic Expression Classes Implementation)
//==============================================================================

// XPathLiteral implementation
std::string XPathLiteral::to_string() const {
    // 在XPath中，字符串字面量用引号包围
    return "\"" + m_value + "\"";
}

// XPathNumber implementation
std::string XPathNumber::to_string() const {
    // 简单实现，实际可能需要更精确的数字到字符串转换
    return std::to_string(m_value);
}

// XPathVariableReference implementation
std::string XPathVariableReference::to_string() const {
    return "$" + m_name;
}

//==============================================================================
// 运算符表达式类实现 (Operator Expression Classes Implementation)
//==============================================================================

// XPathUnaryExpression implementation
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

// XPathBinaryExpression implementation
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

// XPathFunctionCall implementation
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

// XPathNameTest implementation
std::string XPathNameTest::to_string() const {
    if (!m_prefix.empty()) {
        return m_prefix + ":" + m_local_name;
    }
    return m_local_name;
}

// XPathNodeTypeTest implementation
std::string XPathNodeTypeTest::to_string() const {
    std::ostringstream oss;
    switch (m_type) {
        case NodeType::COMMENT:
            oss << "comment()";
            break;
        case NodeType::TEXT:
            oss << "text()";
            break;
        case NodeType::PROCESSING_INSTRUCTION:
            oss << "processing-instruction(";
            if (!m_pi_name.empty()) {
                oss << "'" << m_pi_name << "'";
            }
            oss << ")";
            break;
        case NodeType::NODE:
            oss << "node()";
            break;
    }
    return oss.str();
}

//==============================================================================
// 路径相关类实现 (Path-related Classes Implementation)
//==============================================================================

// XPathPredicate implementation
std::string XPathPredicate::to_string() const {
    return m_expression->to_string();
}

// XPathStep implementation
std::string XPathStep::to_string() const {
    std::ostringstream oss;
    // 输出轴
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
        case XPathAxis::CHILD:
            // child轴是默认轴，通常省略
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
    }
    // 输出节点测试
    oss << m_node_test->to_string();
    // 输出谓词
    for (const auto& predicate : m_predicates) {
        oss << "[" << predicate->to_string() << "]";
    }
    return oss.str();
}

// XPathLocationPath implementation
std::string XPathLocationPath::to_string() const {
    std::ostringstream oss;
    // 如果是绝对路径，以 "/" 开头
    if (m_is_absolute) {
        oss << "/";
    }
    for (size_t i = 0; i < m_steps.size(); ++i) {
        if (i > 0 || (!m_is_absolute && i == 0)) {
            if (i > 0) {
                oss << "/";
            }
        }
        oss << m_steps[i]->to_string();
    }
    return oss.str();
}

// XPathPathExpression implementation
std::string XPathPathExpression::to_string() const {
    std::ostringstream oss;
    if (m_filter_expr) {
        oss << m_filter_expr->to_string();
    }
    for (const auto& step : m_steps) {
        oss << "/" << step->to_string();
    }
    return oss.str();
}

}  // namespace hps