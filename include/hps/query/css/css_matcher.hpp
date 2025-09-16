#pragma once

#include "hps/query/css/css_selector.hpp"

#include <memory>
#include <vector>

namespace hps {

class Document;
class Element;

/**
 * CSS选择器匹配器
 * 提供CSS选择器与DOM元素的匹配功能
 */
class CSSMatcher {
  public:
    // ==================== 公共匹配接口 ====================

    /**
     * 检查元素是否匹配指定的CSS选择器
     * @param element 要检查的元素
     * @param selector CSS选择器
     * @return 如果匹配返回true，否则返回false
     */
    static bool matches(const Element& element, const CSSSelector& selector);

    /**
     * 检查元素是否匹配选择器列表中的任一选择器
     * @param element 要检查的元素
     * @param selector_list 选择器列表
     * @return 如果匹配任一选择器返回true，否则返回false
     */
    static bool matches(const Element& element, const SelectorList& selector_list);

    // ==================== 元素查找接口 ====================

    /**
     * 在指定根元素下查找所有匹配选择器的元素
     * @param root 根元素
     * @param selector CSS选择器
     * @return 匹配的元素列表
     */
    static std::vector<std::shared_ptr<const Element>> find_all(const Element& root, const CSSSelector& selector);

    /**
     * 在指定根元素下查找所有匹配选择器列表的元素
     * @param root 根元素
     * @param selector_list 选择器列表
     * @return 匹配的元素列表（去重）
     */
    static std::vector<std::shared_ptr<const Element>> find_all(const Element& root, const SelectorList& selector_list);

    /**
     * 在文档中查找所有匹配选择器的元素
     * @param document 文档对象
     * @param selector CSS选择器
     * @return 匹配的元素列表
     */
    static std::vector<std::shared_ptr<const Element>> find_all(const Document& document, const CSSSelector& selector);

    /**
     * 在文档中查找所有匹配选择器列表的元素
     * @param document 文档对象
     * @param selector_list 选择器列表
     * @return 匹配的元素列表（去重）
     */
    static std::vector<std::shared_ptr<const Element>> find_all(const Document& document, const SelectorList& selector_list);

    /**
     * 在指定根元素下查找第一个匹配选择器的元素
     * @param root 根元素
     * @param selector CSS选择器
     * @return 第一个匹配的元素，如果没有找到返回nullptr
     */
    static std::shared_ptr<const Element> find_first(const Element& root, const CSSSelector& selector);

    /**
     * 在指定根元素下查找第一个匹配选择器列表的元素
     * @param root 根元素
     * @param selector_list 选择器列表
     * @return 第一个匹配的元素，如果没有找到返回nullptr
     */
    static std::shared_ptr<const Element> find_first(const Element& root, const SelectorList& selector_list);

    /**
     * 在文档中查找第一个匹配选择器的元素
     * @param document 文档对象
     * @param selector CSS选择器
     * @return 第一个匹配的元素，如果没有找到返回nullptr
     */
    static std::shared_ptr<const Element> find_first(const Document& document, const CSSSelector& selector);

    /**
     * 在文档中查找第一个匹配选择器列表的元素
     * @param document 文档对象
     * @param selector_list 选择器列表
     * @return 第一个匹配的元素，如果没有找到返回nullptr
     */
    static std::shared_ptr<const Element> find_first(const Document& document, const SelectorList& selector_list);

  private:
    // ==================== DOM树遍历 ====================

    /**
     * 遍历DOM树并收集匹配的元素
     * @param element 当前元素
     * @param selector CSS选择器
     * @param results 结果容器
     */
    static void traverse_and_match(const Element& element, const CSSSelector& selector, std::vector<std::shared_ptr<const Element>>& results);

    /**
     * 遍历DOM树并收集匹配的元素
     * @param element 当前元素
     * @param selector_list 选择器列表
     * @param results 结果容器
     */
    static void traverse_and_match(const Element& element, const SelectorList& selector_list, std::vector<std::shared_ptr<const Element>>& results);

    // ==================== 选择器类型匹配 ====================

    /**
     * 匹配简单选择器
     * @param element 元素
     * @param selector 简单选择器
     * @return 匹配结果
     */
    static bool matches_simple_selector(const Element& element, const CSSSelector& selector);

    /**
     * 匹配复合选择器
     * @param element 元素
     * @param selector 复合选择器
     * @return 匹配结果
     */
    static bool matches_compound_selector(const Element& element, const CompoundSelector& selector);

    // ==================== 属性匹配 ====================

    /**
     * 匹配属性选择器
     * @param element 元素
     * @param attr_name 属性名
     * @param op 操作符
     * @param value 属性值
     * @return 匹配结果
     */
    static bool matches_attribute(const Element& element, const std::string& attr_name, AttributeOperator op, const std::string& value = "");

    // ==================== DOM导航辅助 ====================

    /**
     * 获取父元素
     * @param element 当前元素
     * @return 父元素，如果没有返回nullptr
     */
    static std::shared_ptr<const Element> get_parent_element(const Element& element);

    /**
     * 获取所有祖先元素
     * @param element 当前元素
     * @return 祖先元素列表（从父到根）
     */
    static std::vector<std::shared_ptr<const Element>> get_ancestor_elements(const Element& element);

    /**
     * 获取前一个兄弟元素
     * @param element 当前元素
     * @return 前一个兄弟元素，如果没有返回nullptr
     */
    static std::shared_ptr<const Element> get_previous_sibling_element(const Element& element);

    /**
     * 获取所有前面的兄弟元素
     * @param element 当前元素
     * @return 前面的兄弟元素列表（按文档顺序）
     */
    static std::vector<std::shared_ptr<const Element>> get_previous_sibling_elements(const Element& element);
};

}  // namespace hps