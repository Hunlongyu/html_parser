#pragma once

#include <memory>
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

//==============================================================================
// 枚举类型 (Enumerations)
//==============================================================================

/**
 * @brief XPath 轴类型枚举
 *
 * 定义了 XPath 1.0 规范中的所有 13 种轴类型，用于在 XML 文档树中导航。
 * 轴指定了从上下文节点开始的搜索方向。
 */
enum class XPathAxis {
    ANCESTOR,            ///< 祖先轴：选择上下文节点的所有祖先节点
    ANCESTOR_OR_SELF,    ///< 祖先或自身轴：选择上下文节点及其所有祖先节点
    ATTRIBUTE,           ///< 属性轴：选择上下文节点的所有属性
    CHILD,               ///< 子轴：选择上下文节点的所有子节点（默认轴）
    DESCENDANT,          ///< 后代轴：选择上下文节点的所有后代节点
    DESCENDANT_OR_SELF,  ///< 后代或自身轴：选择上下文节点及其所有后代节点
    FOLLOWING,           ///< 后续轴：选择文档中上下文节点之后的所有节点
    FOLLOWING_SIBLING,   ///< 后续兄弟轴：选择上下文节点之后的所有兄弟节点
    NAMESPACE,           ///< 命名空间轴：选择上下文节点的所有命名空间节点
    PARENT,              ///< 父轴：选择上下文节点的父节点
    PRECEDING,           ///< 前驱轴：选择文档中上下文节点之前的所有节点
    PRECEDING_SIBLING,   ///< 前驱兄弟轴：选择上下文节点之前的所有兄弟节点
    SELF                 ///< 自身轴：选择上下文节点本身
};

/**
 * @brief XPath 节点测试类型枚举
 *
 * 定义了不同类型的节点测试，用于过滤轴选择的节点。
 */
enum class XPathNodeTestType {
    NAME_TEST,  ///< 名称测试：按节点名称过滤
    NODE_TYPE,  ///< 节点类型测试：按节点类型过滤
    PI_TEST,    ///< 处理指令测试：按处理指令过滤
    WILDCARD    ///< 通配符测试：匹配所有节点
};

//==============================================================================
// 基础抽象类 (Base Abstract Classes)
//==============================================================================

/**
 * @brief XPath 表达式基类
 *
 * 所有 XPath 表达式的抽象基类，定义了表达式的基本接口。
 * XPath 表达式可以求值为四种基本类型之一：节点集、字符串、数字或布尔值。
 */
class XPathExpression {
  public:
    /**
     * @brief 虚析构函数
     */
    virtual ~XPathExpression() = default;

    /**
     * @brief 将表达式转换为字符串表示
     * @return 表达式的字符串表示
     */
    [[nodiscard]] virtual std::string to_string() const = 0;
};

/**
 * @brief XPath 节点测试基类
 *
 * 节点测试用于在轴选择的节点中进行过滤，确定哪些节点符合条件。
 */
class XPathNodeTest {
  public:
    /**
     * @brief 虚析构函数
     */
    virtual ~XPathNodeTest() = default;

    /**
     * @brief 将节点测试转换为字符串表示
     * @return 节点测试的字符串表示
     */
    [[nodiscard]] virtual std::string to_string() const = 0;

    /**
     * @brief 获取节点测试的类型
     * @return 节点测试类型
     */
    [[nodiscard]] virtual XPathNodeTestType type() const = 0;
};

//==============================================================================
// 基本表达式类 (Basic Expression Classes)
//==============================================================================

/**
 * @brief 字面量表达式
 *
 * 表示 XPath 中的字符串字面量，如 "hello" 或 'world'。
 * 字面量在 XPath 中用单引号或双引号包围。
 */
class XPathLiteral : public XPathExpression {
  public:
    /**
     * @brief 构造字面量表达式
     * @param value 字面量的值
     */
    explicit XPathLiteral(std::string value)
        : m_value(std::move(value)) {}

    /**
     * @brief 将字面量转换为字符串表示
     * @return 带引号的字符串字面量
     */
    [[nodiscard]] std::string to_string() const override;

    /**
     * @brief 获取字面量的值
     * @return 字面量值的常量引用
     */
    [[nodiscard]] const std::string& value() const {
        return m_value;
    }

  private:
    std::string m_value;  ///< 字面量的值
};

/**
 * @brief 数字表达式
 *
 * 表示 XPath 中的数字字面量。XPath 中的数字都是双精度浮点数。
 */
class XPathNumber : public XPathExpression {
  public:
    /**
     * @brief 构造数字表达式
     * @param value 数字的值
     */
    explicit XPathNumber(const double value)
        : m_value(value) {}

    /**
     * @brief 将数字转换为字符串表示
     * @return 数字的字符串表示
     */
    [[nodiscard]] std::string to_string() const override;

    /**
     * @brief 获取数字的值
     * @return 数字值
     */
    [[nodiscard]] double value() const {
        return m_value;
    }

  private:
    double m_value;  ///< 数字的值
};

/**
 * @brief 变量引用表达式
 *
 * 表示对 XPath 变量的引用，如 $var。变量名前必须有 $ 符号。
 */
class XPathVariableReference : public XPathExpression {
  public:
    /**
     * @brief 构造变量引用表达式
     * @param name 变量名（不包含 $ 符号）
     */
    explicit XPathVariableReference(std::string name)
        : m_name(std::move(name)) {}

    /**
     * @brief 将变量引用转换为字符串表示
     * @return 带 $ 符号的变量引用
     */
    [[nodiscard]] std::string to_string() const override;

    /**
     * @brief 获取变量名
     * @return 变量名的常量引用
     */
    [[nodiscard]] const std::string& name() const {
        return m_name;
    }

  private:
    std::string m_name;  ///< 变量名（不包含 $ 符号）
};

//==============================================================================
// 运算符表达式类 (Operator Expression Classes)
//==============================================================================

/**
 * @brief 一元表达式
 *
 * 表示一元运算符表达式，如负号运算符 -expr。
 */
class XPathUnaryExpression : public XPathExpression {
  public:
    /**
     * @brief 一元运算符类型
     */
    enum class Operator {
        NEGATE  ///< 负号运算符 (-)
    };

    /**
     * @brief 构造一元表达式
     * @param op 一元运算符
     * @param operand 操作数表达式
     */
    XPathUnaryExpression(Operator op, std::unique_ptr<XPathExpression> operand)
        : m_operator(op),
          m_operand(std::move(operand)) {}

    /**
     * @brief 将一元表达式转换为字符串表示
     * @return 一元表达式的字符串表示
     */
    [[nodiscard]] std::string to_string() const override;

    /**
     * @brief 获取运算符类型
     * @return 运算符类型
     */
    [[nodiscard]] Operator operator_type() const {
        return m_operator;
    }

    /**
     * @brief 获取操作数
     * @return 操作数表达式的常量引用
     */
    [[nodiscard]] const XPathExpression& operand() const {
        return *m_operand;
    }

  private:
    Operator                         m_operator;  ///< 一元运算符
    std::unique_ptr<XPathExpression> m_operand;   ///< 操作数表达式
};

/**
 * @brief 二元表达式
 *
 * 表示二元运算符表达式，如算术运算、比较运算、逻辑运算等。
 */
class XPathBinaryExpression : public XPathExpression {
  public:
    /**
     * @brief 二元运算符类型
     */
    enum class Operator {
        // 逻辑运算符
        OR,   ///< 逻辑或 (or)
        AND,  ///< 逻辑与 (and)

        // 比较运算符
        EQUAL,          ///< 等于 (=)
        NOT_EQUAL,      ///< 不等于 (!=)
        LESS,           ///< 小于 (<)
        LESS_EQUAL,     ///< 小于等于 (<=)
        GREATER,        ///< 大于 (>)
        GREATER_EQUAL,  ///< 大于等于 (>=)

        // 算术运算符
        PLUS,      ///< 加法 (+)
        MINUS,     ///< 减法 (-)
        MULTIPLY,  ///< 乘法 (*)
        DIVIDE,    ///< 除法 (div)
        MODULO,    ///< 取模 (mod)

        // 集合运算符
        UNION  ///< 联合 (|)
    };

    /**
     * @brief 构造二元表达式
     * @param left 左操作数
     * @param op 二元运算符
     * @param right 右操作数
     */
    XPathBinaryExpression(std::unique_ptr<XPathExpression> left, const Operator op, std::unique_ptr<XPathExpression> right)
        : m_left(std::move(left)),
          m_operator(op),
          m_right(std::move(right)) {}

    /**
     * @brief 将二元表达式转换为字符串表示
     * @return 二元表达式的字符串表示
     */
    [[nodiscard]] std::string to_string() const override;

    /**
     * @brief 获取左操作数
     * @return 左操作数的常量引用
     */
    [[nodiscard]] const XPathExpression& left() const {
        return *m_left;
    }

    /**
     * @brief 获取运算符
     * @return 运算符类型
     */
    [[nodiscard]] Operator op() const {
        return m_operator;
    }

    /**
     * @brief 获取右操作数
     * @return 右操作数的常量引用
     */
    [[nodiscard]] const XPathExpression& right() const {
        return *m_right;
    }

  private:
    std::unique_ptr<XPathExpression> m_left;      ///< 左操作数
    Operator                         m_operator;  ///< 二元运算符
    std::unique_ptr<XPathExpression> m_right;     ///< 右操作数
};

/**
 * @brief 函数调用表达式
 *
 * 表示 XPath 函数调用，如 substring(string, start, length)。
 * 支持 XPath 1.0 规范中定义的所有内置函数。
 */
class XPathFunctionCall : public XPathExpression {
  public:
    /**
     * @brief 构造函数调用表达式
     * @param name 函数名
     * @param arguments 参数列表
     */
    XPathFunctionCall(std::string name, std::vector<std::unique_ptr<XPathExpression>> arguments)
        : m_name(std::move(name)),
          m_arguments(std::move(arguments)) {}

    /**
     * @brief 将函数调用转换为字符串表示
     * @return 函数调用的字符串表示
     */
    [[nodiscard]] std::string to_string() const override;

    /**
     * @brief 获取函数名
     * @return 函数名的常量引用
     */
    [[nodiscard]] const std::string& name() const {
        return m_name;
    }

    /**
     * @brief 获取参数列表
     * @return 参数列表的常量引用
     */
    [[nodiscard]] const std::vector<std::unique_ptr<XPathExpression>>& arguments() const {
        return m_arguments;
    }

  private:
    std::string                                   m_name;       ///< 函数名
    std::vector<std::unique_ptr<XPathExpression>> m_arguments;  ///< 参数列表
};

//==============================================================================
// 节点测试类 (Node Test Classes)
//==============================================================================

/**
 * @brief 名称测试
 *
 * 按节点名称进行测试，支持命名空间前缀。
 * 例如：div, html:div, @id 等。
 */
class XPathNameTest : public XPathNodeTest {
  public:
    /**
     * @brief 构造名称测试
     * @param prefix 命名空间前缀（可为空）
     * @param local_name 本地名称
     */
    XPathNameTest(std::string prefix, std::string local_name)
        : m_prefix(std::move(prefix)),
          m_local_name(std::move(local_name)) {}

    /**
     * @brief 将名称测试转换为字符串表示
     * @return 名称测试的字符串表示
     */
    [[nodiscard]] std::string to_string() const override;

    /**
     * @brief 获取节点测试类型
     * @return NAME_TEST
     */
    [[nodiscard]] XPathNodeTestType type() const override {
        return XPathNodeTestType::NAME_TEST;
    }

    /**
     * @brief 获取命名空间前缀
     * @return 命名空间前缀的常量引用
     */
    [[nodiscard]] const std::string& prefix() const {
        return m_prefix;
    }

    /**
     * @brief 获取本地名称
     * @return 本地名称的常量引用
     */
    [[nodiscard]] const std::string& local_name() const {
        return m_local_name;
    }

  private:
    std::string m_prefix;      ///< 命名空间前缀
    std::string m_local_name;  ///< 本地名称
};

/**
 * @brief 通配符测试
 *
 * 匹配所有节点的通配符测试，用 * 表示。
 */
class XPathWildcardTest : public XPathNodeTest {
  public:
    /**
     * @brief 默认构造函数
     */
    XPathWildcardTest() = default;

    /**
     * @brief 将通配符测试转换为字符串表示
     * @return "*"
     */
    [[nodiscard]] std::string to_string() const override {
        return "*";
    }

    /**
     * @brief 获取节点测试类型
     * @return WILDCARD
     */
    [[nodiscard]] XPathNodeTestType type() const override {
        return XPathNodeTestType::WILDCARD;
    }
};

/**
 * @brief 节点类型测试
 *
 * 按节点类型进行测试，如 text()、comment()、node() 等。
 */
class XPathNodeTypeTest : public XPathNodeTest {
  public:
    /**
     * @brief 节点类型枚举
     */
    enum class NodeType {
        COMMENT,                 ///< 注释节点 comment()
        TEXT,                    ///< 文本节点 text()
        PROCESSING_INSTRUCTION,  ///< 处理指令节点 processing-instruction()
        NODE                     ///< 任意节点 node()
    };

    /**
     * @brief 构造节点类型测试
     * @param type 节点类型
     */
    explicit XPathNodeTypeTest(const NodeType type)
        : m_type(type) {}

    /**
     * @brief 构造处理指令节点类型测试
     * @param type 节点类型（必须是 PROCESSING_INSTRUCTION）
     * @param pi_name 处理指令名称
     */
    explicit XPathNodeTypeTest(const NodeType type, std::string pi_name)
        : m_type(type),
          m_pi_name(std::move(pi_name)) {}

    /**
     * @brief 将节点类型测试转换为字符串表示
     * @return 节点类型测试的字符串表示
     */
    [[nodiscard]] std::string to_string() const override;

    /**
     * @brief 获取节点测试类型
     * @return NODE_TYPE
     */
    [[nodiscard]] XPathNodeTestType type() const override {
        return XPathNodeTestType::NODE_TYPE;
    }

    /**
     * @brief 获取节点类型
     * @return 节点类型
     */
    [[nodiscard]] NodeType node_type() const {
        return m_type;
    }

    /**
     * @brief 获取处理指令名称
     * @return 处理指令名称的常量引用
     */
    [[nodiscard]] const std::string& pi_name() const {
        return m_pi_name;
    }

  private:
    NodeType    m_type;     ///< 节点类型
    std::string m_pi_name;  ///< 处理指令名称（仅用于处理指令）
};

//==============================================================================
// 路径相关类 (Path-related Classes)
//==============================================================================

/**
 * @brief 谓词类
 *
 * 表示 XPath 中的谓词，用方括号包围，如 [position()=1] 或 [@id='test']。
 * 谓词用于过滤节点集，只保留满足条件的节点。
 */
class XPathPredicate {
  public:
    /**
     * @brief 构造谓词
     * @param expression 谓词表达式
     */
    explicit XPathPredicate(std::unique_ptr<XPathExpression> expression)
        : m_expression(std::move(expression)) {}

    /**
     * @brief 将谓词转换为字符串表示
     * @return 谓词的字符串表示
     */
    [[nodiscard]] std::string to_string() const;

    /**
     * @brief 获取谓词表达式
     * @return 谓词表达式的常量引用
     */
    [[nodiscard]] const XPathExpression& expression() const {
        return *m_expression;
    }

  private:
    std::unique_ptr<XPathExpression> m_expression;  ///< 谓词表达式
};

/**
 * @brief XPath 步骤类
 *
 * 表示位置路径中的一个步骤，由轴、节点测试和可选的谓词组成。
 * 例如：child::div[@class='content'][1]
 */
class XPathStep {
  public:
    /**
     * @brief 构造 XPath 步骤
     * @param axis 轴类型
     * @param node_test 节点测试
     * @param predicates 谓词列表
     */
    XPathStep(const XPathAxis axis, std::unique_ptr<XPathNodeTest> node_test, std::vector<std::unique_ptr<XPathPredicate>> predicates)
        : m_axis(axis),
          m_node_test(std::move(node_test)),
          m_predicates(std::move(predicates)) {}

    /**
     * @brief 将步骤转换为字符串表示
     * @return 步骤的字符串表示
     */
    [[nodiscard]] std::string to_string() const;

    /**
     * @brief 获取轴类型
     * @return 轴类型
     */
    [[nodiscard]] XPathAxis axis() const {
        return m_axis;
    }

    /**
     * @brief 获取节点测试
     * @return 节点测试的常量引用
     */
    [[nodiscard]] const XPathNodeTest& node_test() const {
        return *m_node_test;
    }

    /**
     * @brief 获取谓词列表
     * @return 谓词列表的常量引用
     */
    [[nodiscard]] const std::vector<std::unique_ptr<XPathPredicate>>& predicates() const {
        return m_predicates;
    }

  private:
    XPathAxis                                    m_axis;        ///< 轴类型
    std::unique_ptr<XPathNodeTest>               m_node_test;   ///< 节点测试
    std::vector<std::unique_ptr<XPathPredicate>> m_predicates;  ///< 谓词列表
};

/**
 * @brief 位置路径表达式
 *
 * 表示 XPath 位置路径，由一系列步骤组成。
 * 可以是绝对路径（以 / 开头）或相对路径。
 * 例如：/html/body/div 或 div/p[@class='text']
 */
class XPathLocationPath : public XPathExpression {
  public:
    /**
     * @brief 构造位置路径表达式
     * @param steps 步骤列表
     * @param is_absolute 是否为绝对路径
     */
    XPathLocationPath(std::vector<std::unique_ptr<XPathStep>> steps, bool is_absolute = false)
        : m_steps(std::move(steps)),
          m_is_absolute(is_absolute) {}

    /**
     * @brief 将位置路径转换为字符串表示
     * @return 位置路径的字符串表示
     */
    [[nodiscard]] std::string to_string() const override;

    /**
     * @brief 判断是否为绝对路径
     * @return 如果是绝对路径返回 true，否则返回 false
     */
    [[nodiscard]] bool is_absolute() const {
        return m_is_absolute;
    }

    /**
     * @brief 获取步骤列表
     * @return 步骤列表的常量引用
     */
    [[nodiscard]] const std::vector<std::unique_ptr<XPathStep>>& steps() const {
        return m_steps;
    }

  private:
    std::vector<std::unique_ptr<XPathStep>> m_steps;        ///< 步骤列表
    bool                                    m_is_absolute;  ///< 是否为绝对路径
};

/**
 * @brief 路径表达式
 *
 * 表示过滤表达式后跟位置路径的复合表达式。
 * 例如：(//div[@class='content'])/p[1]
 */
class XPathPathExpression : public XPathExpression {
  public:
    /**
     * @brief 构造路径表达式
     * @param filter_expr 过滤表达式
     * @param steps 后续步骤列表
     */
    XPathPathExpression(std::unique_ptr<XPathExpression> filter_expr, std::vector<std::unique_ptr<XPathStep>> steps)
        : m_filter_expr(std::move(filter_expr)),
          m_steps(std::move(steps)) {}

    /**
     * @brief 将路径表达式转换为字符串表示
     * @return 路径表达式的字符串表示
     */
    [[nodiscard]] std::string to_string() const override;

    /**
     * @brief 获取过滤表达式
     * @return 过滤表达式的指针（可能为空）
     */
    [[nodiscard]] const XPathExpression* filter_expr() const {
        return m_filter_expr.get();
    }

    /**
     * @brief 获取步骤列表
     * @return 步骤列表的常量引用
     */
    [[nodiscard]] const std::vector<std::unique_ptr<XPathStep>>& steps() const {
        return m_steps;
    }

  private:
    std::unique_ptr<XPathExpression>        m_filter_expr;  ///< 过滤表达式
    std::vector<std::unique_ptr<XPathStep>> m_steps;        ///< 后续步骤列表
};

}  // namespace hps