#include "hps/query/xpath/xpath_query.hpp"
#include "hps/query/xpath/xpath_parser.hpp"
#include "hps/query/xpath/xpath_evaluator.hpp"

#include <utility>
#include <variant>

namespace hps {

XPathQuery::XPathQuery(const std::string_view expression)
    : m_expression(expression) {
    parse_expression();
}

XPathQuery::XPathQuery(const XPathQuery& other)
    : m_expression(other.m_expression) {
    // 重新解析，避免 AST 深拷贝
    parse_expression();
}

XPathQuery::XPathQuery(XPathQuery&& other) noexcept
    : m_expression(std::move(other.m_expression)),
      m_parsed_expression(std::move(other.m_parsed_expression)) {}

XPathQuery& XPathQuery::operator=(const XPathQuery& other) {
    if (this != &other) {
        m_expression = other.m_expression;
        parse_expression();
    }
    return *this;
}

XPathQuery& XPathQuery::operator=(XPathQuery&& other) noexcept {
    if (this != &other) {
        m_expression        = std::move(other.m_expression);
        m_parsed_expression = std::move(other.m_parsed_expression);
    }
    return *this;
}

XPathQuery::~XPathQuery() = default;

// 静态工厂方法
XPathQuery XPathQuery::compile(std::string expression) {
    return XPathQuery(expression);
}

// 查询方法
ElementQuery XPathQuery::select(const Element& element) const {
    if (!m_parsed_expression) {
        return ElementQuery{};
    }
    XPathEvaluator evaluator(element);
    XPathValue     value = evaluator.evaluate(*m_parsed_expression);

    if (auto nodes = std::get_if<std::vector<std::shared_ptr<const Element>>>(&value)) {
        return ElementQuery(std::vector<std::shared_ptr<const Element>>{std::make_move_iterator(nodes->begin()),
                                                                        std::make_move_iterator(nodes->end())});
    }
    // 非节点集返回空结果
    return ElementQuery{};
}

ElementQuery XPathQuery::select(const Document& document) const {
    if (!m_parsed_expression) {
        return ElementQuery{};
    }
    XPathEvaluator evaluator(document);
    XPathValue     value = evaluator.evaluate(*m_parsed_expression);

    if (auto nodes = std::get_if<std::vector<std::shared_ptr<const Element>>>(&value)) {
        return ElementQuery(std::vector<std::shared_ptr<const Element>>{std::make_move_iterator(nodes->begin()),
                                                                        std::make_move_iterator(nodes->end())});
    }
    // 非节点集返回空结果
    return ElementQuery{};
}

// 获取表达式字符串
const std::string& XPathQuery::expression() const {
    return m_expression;
}

void XPathQuery::parse_expression() {
    try {
        XPathParser parser(m_expression);
        m_parsed_expression = parser.parse();
    } catch (...) {
        // 解析失败则置空，select 返回空结果
        m_parsed_expression.reset();
    }
}

}  // namespace hps