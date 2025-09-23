#pragma once

#include "hps/query/element_query.hpp"

#include <memory>
#include <string>

namespace hps {
class Element;
class Document;

class XPathExpression;

class XPathQuery {
  public:
    explicit XPathQuery(const std::string_view expression);
    XPathQuery(const XPathQuery& other);
    XPathQuery(XPathQuery&& other) noexcept;
    XPathQuery& operator=(const XPathQuery& other);
    XPathQuery& operator=(XPathQuery&& other) noexcept;
    ~XPathQuery();

    // 静态工厂方法
    static XPathQuery compile(std::string expression);

    // 查询方法
    ElementQuery select(const Element& element) const;
    ElementQuery select(const Document& document) const;

    // 获取表达式字符串
    const std::string& expression() const;

  private:
    std::string                      m_expression;
    std::unique_ptr<XPathExpression> m_parsed_expression;

    void parse_expression();
};

}  // namespace hps