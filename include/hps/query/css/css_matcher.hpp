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
};

}  // namespace hps