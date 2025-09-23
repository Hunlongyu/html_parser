#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace hps {

//==============================================================================
// 前向声明 (Forward Declarations)
//==============================================================================

class XPathExpression;
class XPathStep;
class XPathNodeTest;
class XPathPredicate;
class XPathFunctionCall;
class XPathLocationPath;

//==============================================================================
// 枚举类型 (Enumerations)
//==============================================================================

/**
 * @brief XPath 轴类型枚举
 *
 * 定义了 XPath 1.0 规范中的所有 13 种轴类型。
 */
enum class XPathAxis { ANCESTOR, ANCESTOR_OR_SELF, ATTRIBUTE, CHILD, DESCENDANT, DESCENDANT_OR_SELF, FOLLOWING, FOLLOWING_SIBLING, NAMESPACE, PARENT, PRECEDING, PRECEDING_SIBLING, SELF };

/**
 * @brief XPath 节点测试类型枚举
 *
 * 定义了不同类型的节点测试，用于过滤轴选择的节点。
 */
enum class XPathNodeTestType {
    NAME_TEST,  ///< 名称测试：按节点名称过滤
    NODE_TYPE,  ///< 节点类型测试：如 text(), node()
    PI_TEST,    ///< 处理指令测试：processing-instruction()
    WILDCARD    ///< 通配符测试：*
};

//==============================================================================
// 基础抽象类 (Base Abstract Classes)
//==============================================================================

/**
 * @brief XPath 表达式基类
 *
 * 所有 XPath 表达式的抽象基类。
 */
class XPathExpression {
  public:
    virtual ~XPathExpression()                          = default;
    [[nodiscard]] virtual std::string to_string() const = 0;
};

/**
 * @brief XPath 节点测试基类
 *
 * 节点测试用于在轴选择的节点中进行过滤。
 */
class XPathNodeTest {
  public:
    virtual ~XPathNodeTest()                                  = default;
    [[nodiscard]] virtual std::string       to_string() const = 0;
    [[nodiscard]] virtual XPathNodeTestType type() const      = 0;
};

//==============================================================================
// 基本表达式类 (Basic Expression Classes)
//==============================================================================

/**
 * @brief 字面量表达式
 *
 * 表示 XPath 中的字符串字面量。
 */
class XPathLiteral : public XPathExpression {
  public:
    explicit XPathLiteral(std::string value)
        : m_value(std::move(value)) {}
    [[nodiscard]] std::string        to_string() const override;
    [[nodiscard]] const std::string& value() const {
        return m_value;
    }

  private:
    std::string m_value;
};

/**
 * @brief 数字表达式
 *
 * 表示 XPath 中的数字字面量。
 */
class XPathNumber : public XPathExpression {
  public:
    explicit XPathNumber(const double value)
        : m_value(value) {}
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] double      value() const {
        return m_value;
    }

  private:
    double m_value;
};

/**
 * @brief 变量引用表达式
 *
 * 表示对 XPath 变量的引用，如 $var。
 */
class XPathVariableReference : public XPathExpression {
  public:
    explicit XPathVariableReference(std::string name)
        : m_name(std::move(name)) {}
    [[nodiscard]] std::string        to_string() const override;
    [[nodiscard]] const std::string& name() const {
        return m_name;
    }

  private:
    std::string m_name;
};

//==============================================================================
// 运算符表达式类 (Operator Expression Classes)
//==============================================================================

/**
 * @brief 一元表达式
 *
 * 表示一元运算符表达式，如 -expr。
 */
class XPathUnaryExpression : public XPathExpression {
  public:
    enum class Operator { NEGATE };

    XPathUnaryExpression(Operator op, std::unique_ptr<XPathExpression> operand)
        : m_operator(op),
          m_operand(std::move(operand)) {}
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] Operator    operator_type() const {
        return m_operator;
    }
    [[nodiscard]] const XPathExpression& operand() const {
        return *m_operand;
    }

  private:
    Operator                         m_operator;
    std::unique_ptr<XPathExpression> m_operand;
};

/**
 * @brief 二元表达式
 *
 * 表示二元运算符表达式。
 */
class XPathBinaryExpression : public XPathExpression {
  public:
    enum class Operator { OR, AND, EQUAL, NOT_EQUAL, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL, PLUS, MINUS, MULTIPLY, DIVIDE, MODULO, UNION };

    XPathBinaryExpression(std::unique_ptr<XPathExpression> left, const Operator op, std::unique_ptr<XPathExpression> right)
        : m_left(std::move(left)),
          m_operator(op),
          m_right(std::move(right)) {}
    [[nodiscard]] std::string            to_string() const override;
    [[nodiscard]] const XPathExpression& left() const {
        return *m_left;
    }
    [[nodiscard]] Operator op() const {
        return m_operator;
    }
    [[nodiscard]] const XPathExpression& right() const {
        return *m_right;
    }

  private:
    std::unique_ptr<XPathExpression> m_left;
    Operator                         m_operator;
    std::unique_ptr<XPathExpression> m_right;
};

/**
 * @brief 函数调用表达式
 *
 * 表示 XPath 函数调用。
 */
class XPathFunctionCall : public XPathExpression {
  public:
    XPathFunctionCall(std::string name, std::vector<std::unique_ptr<XPathExpression>> arguments)
        : m_name(std::move(name)),
          m_arguments(std::move(arguments)) {}
    [[nodiscard]] std::string        to_string() const override;
    [[nodiscard]] const std::string& name() const {
        return m_name;
    }
    [[nodiscard]] const std::vector<std::unique_ptr<XPathExpression>>& arguments() const {
        return m_arguments;
    }

  private:
    std::string                                   m_name;
    std::vector<std::unique_ptr<XPathExpression>> m_arguments;
};

//==============================================================================
// 节点测试类 (Node Test Classes)
//==============================================================================

/**
 * @brief 名称测试
 *
 * 按节点名称进行测试，如 div, @id。
 */
class XPathNameTest : public XPathNodeTest {
  public:
    // 原有构造函数：带前缀和本地名称
    XPathNameTest(std::string prefix, std::string local_name)
        : m_prefix(std::move(prefix)),
          m_local_name(std::move(local_name)) {}

    // 新增便利构造函数：只有本地名称（无前缀）
    explicit XPathNameTest(std::string name)
        : m_prefix(""),
          m_local_name(std::move(name)) {}

    [[nodiscard]] std::string       to_string() const override;
    [[nodiscard]] XPathNodeTestType type() const override {
        return XPathNodeTestType::NAME_TEST;
    }
    [[nodiscard]] const std::string& prefix() const {
        return m_prefix;
    }
    [[nodiscard]] const std::string& local_name() const {
        return m_local_name;
    }

  private:
    std::string m_prefix;
    std::string m_local_name;
};

/**
 * @brief 通配符测试
 *
 * 匹配所有节点的通配符测试，*。
 */
class XPathWildcardTest : public XPathNodeTest {
  public:
    XPathWildcardTest() = default;
    [[nodiscard]] std::string to_string() const override {
        return "*";
    }
    [[nodiscard]] XPathNodeTestType type() const override {
        return XPathNodeTestType::WILDCARD;
    }
};

/**
 * @brief 节点类型测试
 *
 * 按节点类型进行测试，如 text(), comment(), node()。
 */
class XPathNodeTypeTest : public XPathNodeTest {
  public:
    enum class NodeType { COMMENT, TEXT, NODE };

    explicit XPathNodeTypeTest(const NodeType type)
        : m_type(type) {}
    [[nodiscard]] std::string       to_string() const override;
    [[nodiscard]] XPathNodeTestType type() const override {
        return XPathNodeTestType::NODE_TYPE;
    }
    [[nodiscard]] NodeType node_type() const {
        return m_type;
    }

  private:
    NodeType m_type;
};

/**
 * @brief NEW: 处理指令测试 (Improvement)
 *
 * 独立表示 processing-instruction() 测试，增强类型安全。
 */
class XPathPITest : public XPathNodeTest {
  public:
    explicit XPathPITest(std::string target)
        : m_target(std::move(target)) {}
    [[nodiscard]] std::string       to_string() const override;
    [[nodiscard]] XPathNodeTestType type() const override {
        return XPathNodeTestType::PI_TEST;
    }
    [[nodiscard]] const std::string& target() const {
        return m_target;
    }

  private:
    std::string m_target;
};

//==============================================================================
// 路径相关类 (Path-related Classes)
//==============================================================================

/**
 * @brief 谓词类
 *
 * 表示 XPath 中的谓词，如 [position()=1]。
 */
class XPathPredicate {
  public:
    explicit XPathPredicate(std::unique_ptr<XPathExpression> expression)
        : m_expression(std::move(expression)) {}
    [[nodiscard]] std::string            to_string() const;
    [[nodiscard]] const XPathExpression& expression() const {
        return *m_expression;
    }

  private:
    std::unique_ptr<XPathExpression> m_expression;
};

/**
 * @brief XPath 步骤类
 *
 * 表示位置路径中的一个步骤。
 */
class XPathStep {
  public:
    XPathStep(const XPathAxis axis, std::unique_ptr<XPathNodeTest> node_test, std::vector<std::unique_ptr<XPathPredicate>> predicates)
        : m_axis(axis),
          m_node_test(std::move(node_test)),
          m_predicates(std::move(predicates)) {}
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] XPathAxis   axis() const {
        return m_axis;
    }
    [[nodiscard]] const XPathNodeTest& node_test() const {
        return *m_node_test;
    }
    [[nodiscard]] const std::vector<std::unique_ptr<XPathPredicate>>& predicates() const {
        return m_predicates;
    }

  private:
    XPathAxis                                    m_axis;
    std::unique_ptr<XPathNodeTest>               m_node_test;
    std::vector<std::unique_ptr<XPathPredicate>> m_predicates;
};

/**
 * @brief 位置路径表达式
 *
 * 表示 XPath 位置路径，由一系列步骤组成。
 */
class XPathLocationPath : public XPathExpression {
  public:
    XPathLocationPath(std::vector<std::unique_ptr<XPathStep>> steps, bool is_absolute = false)
        : m_steps(std::move(steps)),
          m_is_absolute(is_absolute) {}
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] bool        is_absolute() const {
        return m_is_absolute;
    }
    [[nodiscard]] const std::vector<std::unique_ptr<XPathStep>>& steps() const {
        return m_steps;
    }

  private:
    std::vector<std::unique_ptr<XPathStep>> m_steps;
    bool                                    m_is_absolute;
};

/**
 * @brief 过滤表达式
 *
 * 表示主表达式后跟零个或多个谓词的过滤表达式。
 */
class XPathFilterExpression : public XPathExpression {
  public:
    XPathFilterExpression(std::unique_ptr<XPathExpression> primary_expr, std::vector<std::unique_ptr<XPathPredicate>> predicates)
        : m_primary_expr(std::move(primary_expr)),
          m_predicates(std::move(predicates)) {}
    [[nodiscard]] std::string            to_string() const override;
    [[nodiscard]] const XPathExpression* primary_expr() const {
        return m_primary_expr.get();
    }
    [[nodiscard]] const std::vector<std::unique_ptr<XPathPredicate>>& predicates() const {
        return m_predicates;
    }
    [[nodiscard]] bool has_predicates() const {
        return !m_predicates.empty();
    }

  private:
    std::unique_ptr<XPathExpression>             m_primary_expr;
    std::vector<std::unique_ptr<XPathPredicate>> m_predicates;
};

/**
 * @brief NEW: 链式路径表达式 (Major Improvement)
 *
 * 对应于语法 'FilterExpr / RelativeLocationPath'。
 * 例如：($var)/foo 或 (a|b)//c
 */
class XPathChainedPath : public XPathExpression {
  public:
    enum class Separator { SLASH, DOUBLE_SLASH };

    XPathChainedPath(std::unique_ptr<XPathExpression> filter_expr, Separator separator, std::unique_ptr<XPathLocationPath> relative_path)
        : m_filter_expr(std::move(filter_expr)),
          m_separator(separator),
          m_relative_path(std::move(relative_path)) {
        if (relative_path && relative_path->is_absolute()) {
            throw std::invalid_argument("Chained path must use a relative location path.");
        }
    }

    [[nodiscard]] std::string to_string() const override;

    [[nodiscard]] const XPathExpression& filter_expr() const {
        return *m_filter_expr;
    }
    [[nodiscard]] Separator separator() const {
        return m_separator;
    }
    [[nodiscard]] const XPathLocationPath& relative_path() const {
        return *m_relative_path;
    }

  private:
    std::unique_ptr<XPathExpression>   m_filter_expr;
    Separator                          m_separator;
    std::unique_ptr<XPathLocationPath> m_relative_path;
};

}  // namespace hps