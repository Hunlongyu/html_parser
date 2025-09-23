#pragma once

#include "hps/query/element_query.hpp"
#include "hps/query/xpath/xpath_ast.hpp"

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace hps {
class Element;
class Document;

// XPath 求值结果类型
using XPathValue = std::variant<std::vector<std::shared_ptr<const Element>>,  // 节点集
                                std::string,                                  // 字符串
                                double,                                       // 数字
                                bool                                          // 布尔值
                                >;

class XPathEvaluator {
  public:
    explicit XPathEvaluator(const Element& context_node);
    explicit XPathEvaluator(const Document& document);

    XPathValue evaluate(const XPathExpression& expression);

  private:
    std::shared_ptr<const Element> m_context_node;
    bool                           m_is_document_context;

    // 求值函数
    XPathValue evaluate_expression(const XPathExpression& expr);
    XPathValue evaluate_binary_expression(const XPathBinaryExpression& expr);
    XPathValue evaluate_function_call(const XPathFunctionCall& expr);
    XPathValue evaluate_literal(const XPathLiteral& expr);
    XPathValue evaluate_number(const XPathNumber& expr);
    XPathValue evaluate_variable_reference(const XPathVariableReference& expr);
    XPathValue evaluate_path_expression(const XPathPathExpression& expr);
    XPathValue evaluate_location_path(const XPathLocationPath& expr);

    // 步骤求值
    std::vector<std::shared_ptr<const Element>> evaluate_step(const std::vector<std::shared_ptr<const Element>>& context_nodes, const XPathStep& step);

    // 轴求值
    std::vector<std::shared_ptr<const Element>> evaluate_axis(const std::vector<std::shared_ptr<const Element>>& context_nodes, XPathAxis axis);

    // 节点测试求值
    std::vector<std::shared_ptr<const Element>> evaluate_node_test(const std::vector<std::shared_ptr<const Element>>& nodes, const XPathNodeTest& node_test);

    // 谓词求值
    std::vector<std::shared_ptr<const Element>> evaluate_predicates(const std::vector<std::shared_ptr<const Element>>& nodes, const std::vector<std::unique_ptr<XPathPredicate>>& predicates);

    // 类型转换函数
    static std::vector<std::shared_ptr<const Element>> to_node_set(const XPathValue& value);
    static std::string                                 to_string(const XPathValue& value);
    static double                                      to_number(const XPathValue& value);
    static bool                                        to_boolean(const XPathValue& value);

    // 函数库
    XPathValue evaluate_function(const std::string& name, const std::vector<std::unique_ptr<XPathExpression>>& arguments);

    // XPath 核心函数
    XPathValue function_last(const std::vector<XPathValue>& args);
    XPathValue function_position(const std::vector<XPathValue>& args);
    XPathValue function_count(const std::vector<XPathValue>& args);
    XPathValue function_id(const std::vector<XPathValue>& args);
    XPathValue function_local_name(const std::vector<XPathValue>& args);
    XPathValue function_namespace_uri(const std::vector<XPathValue>& args);
    XPathValue function_name(const std::vector<XPathValue>& args);
    XPathValue function_string(const std::vector<XPathValue>& args);
    XPathValue function_concat(const std::vector<XPathValue>& args);
    XPathValue function_starts_with(const std::vector<XPathValue>& args);
    XPathValue function_contains(const std::vector<XPathValue>& args);
    XPathValue function_substring_before(const std::vector<XPathValue>& args);
    XPathValue function_substring_after(const std::vector<XPathValue>& args);
    XPathValue function_substring(const std::vector<XPathValue>& args);
    XPathValue function_string_length(const std::vector<XPathValue>& args);
    XPathValue function_normalize_space(const std::vector<XPathValue>& args);
    XPathValue function_translate(const std::vector<XPathValue>& args);
    XPathValue function_boolean(const std::vector<XPathValue>& args);
    XPathValue function_not(const std::vector<XPathValue>& args);
    XPathValue function_true(const std::vector<XPathValue>& args);
    XPathValue function_false(const std::vector<XPathValue>& args);
    XPathValue function_lang(const std::vector<XPathValue>& args);
    XPathValue function_number(const std::vector<XPathValue>& args);
    XPathValue function_sum(const std::vector<XPathValue>& args);
    XPathValue function_floor(const std::vector<XPathValue>& args);
    XPathValue function_ceiling(const std::vector<XPathValue>& args);
    XPathValue function_round(const std::vector<XPathValue>& args);
};

}  // namespace hps