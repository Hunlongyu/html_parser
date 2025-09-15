#pragma once

#include "css_selector.hpp"
#include "hps/core/element.hpp"
#include "hps/query/element_query.hpp"

#include <string_view>
#include <vector>

namespace hps {

/**
 * @brief CSS查询接口类
 * 提供便捷的CSS选择器查询功能
 */
class CSSQuery {
  public:
    /**
     * @brief 在指定根节点下查询第一个匹配的元素
     * @param selector CSS选择器字符串
     * @param root 查询的根节点
     * @return 匹配的元素，无匹配返回nullptr
     */
    static std::shared_ptr<const Element> querySelector(std::string_view selector, const Node* root);

    /**
     * @brief 在指定根节点下查询所有匹配的元素
     * @param selector CSS选择器字符串
     * @param root 查询的根节点
     * @return 匹配的元素列表
     */
    static std::vector<std::shared_ptr<const Element>> querySelectorAll(std::string_view selector, const Node* root);

    /**
     * @brief 创建ElementQuery对象进行链式查询
     * @param selector CSS选择器字符串
     * @param root 查询的根节点
     * @return ElementQuery对象
     */
    static ElementQuery query(std::string_view selector, const Node* root);

    /**
     * @brief 验证CSS选择器语法是否正确
     * @param selector CSS选择器字符串
     * @return 语法是否正确
     */
    static bool isValidSelector(std::string_view selector);

    /**
     * @brief 解析CSS选择器为AST
     * @param selector CSS选择器字符串
     * @return 选择器AST，解析失败返回nullptr
     */
    static std::unique_ptr<SelectorList> parseSelector(std::string_view selector);

  private:
    CSSQuery() = delete;
};
}  // namespace hps