#pragma once

#include "css_selector.hpp"
#include <vector>

namespace hps {

class Element;
class Document;

// CSS匹配器 - 负责将选择器与DOM元素进行匹配
class CSSMatcher {
  public:
    // 检查元素是否匹配选择器
    static bool matches(const Element& element, const CSSSelector& selector);
    
    // 检查元素是否匹配选择器列表中的任一选择器
    static bool matches(const Element& element, const SelectorList& selector_list);
    
    // 在指定根节点下查找所有匹配的元素
    static std::vector<const Element*> find_all(const Element& root, const CSSSelector& selector);
    static std::vector<const Element*> find_all(const Element& root, const SelectorList& selector_list);
    
    // 在文档中查找所有匹配的元素
    static std::vector<const Element*> find_all(const Document& document, const CSSSelector& selector);
    static std::vector<const Element*> find_all(const Document& document, const SelectorList& selector_list);
    
    // 查找第一个匹配的元素
    static const Element* find_first(const Element& root, const CSSSelector& selector);
    static const Element* find_first(const Element& root, const SelectorList& selector_list);
    static const Element* find_first(const Document& document, const CSSSelector& selector);
    static const Element* find_first(const Document& document, const SelectorList& selector_list);
    
  private:
    // 递归遍历DOM树查找匹配元素
    static void traverse_and_match(const Element& element, 
                                  const CSSSelector& selector,
                                  std::vector<const Element*>& results);
    
    static void traverse_and_match(const Element& element, 
                                  const SelectorList& selector_list,
                                  std::vector<const Element*>& results);
    
    // 匹配具体的选择器类型
    static bool matches_simple_selector(const Element& element, const CSSSelector& selector);
    static bool matches_compound_selector(const Element& element, const CompoundSelector& selector);
    static bool matches_combinator_selector(const Element& element, const CombinatorSelector& selector);
    
    // 匹配后代选择器
    static bool matches_descendant(const Element& element, const CSSSelector& ancestor_selector);
    
    // 匹配子选择器
    static bool matches_child(const Element& element, const CSSSelector& parent_selector);
    
    // 属性匹配辅助函数
    static bool matches_attribute(const Element& element, 
                                 const std::string& attr_name,
                                 AttributeOperator op,
                                 const std::string& value);
    
    // DOM导航辅助函数
    static const Element* get_parent_element(const Element& element);
    static std::vector<const Element*> get_ancestor_elements(const Element& element);
};

}  // namespace hps