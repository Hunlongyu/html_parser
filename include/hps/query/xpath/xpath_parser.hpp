#pragma once

#include "hps/query/xpath/xpath_ast.hpp"
#include "hps/query/xpath/xpath_lexer.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace hps {

/**
 * @brief XPath 解析器异常类
 */
class XPathParseError : public std::runtime_error {
  public:
    explicit XPathParseError(const std::string& message)
        : std::runtime_error("XPath Parse Error: " + message) {}
};

/**
 * @brief XPath 表达式解析器
 *
 * 负责将 XPath 表达式字符串解析为 AST（抽象语法树）。
 * 支持 XPath 1.0 规范的核心语法特性。
 */
class XPathParser {
  public:
    /**
     * @brief 构造函数
     * @param expression XPath 表达式字符串
     */
    explicit XPathParser(std::string_view expression);

    /**
     * @brief 解析 XPath 表达式
     * @return 解析后的 AST 根节点
     * @throws XPathParseError 解析失败时抛出异常
     */
    std::unique_ptr<XPathExpression> parse();

  private:
    XPathLexer m_lexer;           ///< 词法分析器
    XPathToken m_current_token;   ///< 当前 token
    bool       m_token_consumed;  ///< 当前 token 是否已被消费

    // ========== Token 管理 ==========

    /**
     * @brief 获取当前 token
     */
    const XPathToken& current_token();

    /**
     * @brief 前进到下一个 token
     */
    void advance();

    /**
     * @brief 检查当前 token 是否为指定类型
     */
    bool match(XPathTokenType type) const;

    /**
     * @brief 检查当前 token 是否为指定类型之一
     */
    bool match_any(const std::vector<XPathTokenType>& types) const;

    /**
     * @brief 消费指定类型的 token，如果不匹配则抛出异常
     */
    void consume(XPathTokenType type);

    /**
     * @brief 消费指定类型的 token，如果匹配则返回 true 并前进
     */
    bool consume_if(XPathTokenType type);

    // ========== 表达式解析（按优先级从低到高） ==========

    /**
     * @brief 解析顶层表达式（Or 表达式）
     */
    std::unique_ptr<XPathExpression> parse_or_expression();

    /**
     * @brief 解析 And 表达式
     */
    std::unique_ptr<XPathExpression> parse_and_expression();

    /**
     * @brief 解析相等性表达式（= !=）
     */
    std::unique_ptr<XPathExpression> parse_equality_expression();

    /**
     * @brief 解析关系表达式（< <= > >=）
     */
    std::unique_ptr<XPathExpression> parse_relational_expression();

    /**
     * @brief 解析加法表达式（+ -）
     */
    std::unique_ptr<XPathExpression> parse_additive_expression();

    /**
     * @brief 解析乘法表达式（* div mod）
     */
    std::unique_ptr<XPathExpression> parse_multiplicative_expression();

    /**
     * @brief 解析一元表达式（-）
     */
    std::unique_ptr<XPathExpression> parse_unary_expression();

    /**
     * @brief 解析联合表达式（|）
     */
    std::unique_ptr<XPathExpression> parse_union_expression();

    /**
     * @brief 解析路径表达式
     */
    std::unique_ptr<XPathExpression> parse_path_expression();

    /**
     * @brief 解析过滤表达式
     */
    std::unique_ptr<XPathExpression> parse_filter_expression();

    /**
     * @brief 解析基本表达式
     */
    std::unique_ptr<XPathExpression> parse_primary_expression();

    // ========== 路径相关解析 ==========
    bool is_location_path_start();
    /**
     * @brief 解析位置路径
     */
    std::unique_ptr<XPathLocationPath> parse_location_path();

    /**
     * @brief 解析绝对位置路径
     */
    std::unique_ptr<XPathLocationPath> parse_absolute_location_path();

    /**
     * @brief 解析相对位置路径
     */
    std::unique_ptr<XPathLocationPath> parse_relative_location_path();

    std::vector<std::unique_ptr<XPathStep>> parse_relative_location_path_steps();

    bool is_step_start();
    /**
     * @brief 解析路径步骤
     */
    std::unique_ptr<XPathStep> parse_step();

    /**
     * @brief 解析缩写步骤（. 和 ..）
     */
    std::unique_ptr<XPathStep> parse_abbreviated_step();

    // ========== 节点测试和谓词解析 ==========

    /**
     * @brief 解析节点测试
     */
    std::unique_ptr<XPathNodeTest> parse_node_test();

    /**
     * @brief 解析谓词列表
     */
    std::vector<std::unique_ptr<XPathPredicate>> parse_predicates();

    /**
     * @brief 解析单个谓词
     */
    std::unique_ptr<XPathPredicate> parse_predicate();

    // ========== 函数调用解析 ==========

    /**
     * @brief 解析函数调用
     */
    std::unique_ptr<XPathFunctionCall> parse_function_call();

    /**
     * @brief 解析函数参数列表
     */
    std::vector<std::unique_ptr<XPathExpression>> parse_argument_list();

    // ========== 辅助函数 ==========

    /**
     * @brief 检查 token 是否为轴说明符
     */
    bool is_axis_specifier();

    /**
     * @brief 解析轴说明符
     */
    XPathAxis parse_axis_specifier();

    /**
     * @brief 检查 token 是否为节点类型测试
     */
    bool is_node_type_test();

    /**
     * @brief 解析节点类型测试
     */
    // 修改返回类型为基类指针
    std::unique_ptr<XPathNodeTest> parse_node_type_test();

    /**
     * @brief 检查 token 是否为操作符
     */
    bool is_operator() const;

    /**
     * @brief 将 token 转换为二元操作符
     */
    XPathBinaryExpression::Operator token_to_binary_operator(XPathTokenType type) const;

    /**
     * @brief 解析字符串字面量
     */
    std::string parse_string_literal();

    /**
     * @brief 解析数字字面量
     */
    double parse_number_literal();

    /**
     * @brief 创建错误信息
     */
    std::string create_error_message(const std::string& expected) const;

    /**
     * @brief 抛出解析错误
     */
    [[noreturn]] void throw_parse_error(const std::string& message) const;
};

}  // namespace hps
