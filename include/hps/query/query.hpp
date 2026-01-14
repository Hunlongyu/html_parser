#pragma once
#include "hps/query/element_query.hpp"

#include <string_view>

namespace hps {
class Document;
class Element;

/**
 * @brief 查询工具类 Query utility class
 *
 * 提供静态方法用于在文档或元素中执行 CSS 和 XPath 查询
 * Provides static methods for executing CSS and XPath queries in documents or elements
 */
class Query {
  public:
    /**
     * @brief 在指定元素中使用 CSS 选择器查询子元素 Query child elements using CSS selector in specified element
     * @param element 目标元素 Target element
     * @param selector CSS 选择器 CSS selector
     * @return 匹配的元素查询对象 ElementQuery object containing matched elements
     */
    static ElementQuery css(const Element& element, std::string_view selector);

    /**
     * @brief 在文档中使用 CSS 选择器查询元素 Query elements using CSS selector in document
     * @param document 目标文档 Target document
     * @param selector CSS 选择器 CSS selector
     * @return 匹配的元素查询对象 ElementQuery object containing matched elements
     */
    static ElementQuery css(const Document& document, std::string_view selector);


};
}  // namespace hps