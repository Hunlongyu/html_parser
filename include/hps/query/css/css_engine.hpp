#pragma once

#include "css_selector.hpp"
#include "css_parser.hpp"
#include "css_matcher.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace hps {

class Element;
class Document;
class ElementQuery;

// CSS查询引擎 - 提供高级CSS查询接口
class CSSEngine {
  public:
    // 在指定元素下执行CSS查询
    static ElementQuery query(const Element& element, std::string_view selector);
    
    // 在文档中执行CSS查询
    static ElementQuery query(const Document& document, std::string_view selector);
    
    // 查找第一个匹配的元素
    static const Element* query_first(const Element& element, std::string_view selector);
    static const Element* query_first(const Document& document, std::string_view selector);
    
    // 查找所有匹配的元素
    static std::vector<const Element*> query_all(const Element& element, std::string_view selector);
    static std::vector<const Element*> query_all(const Document& document, std::string_view selector);
    
    // 检查元素是否匹配选择器
    static bool matches(const Element& element, std::string_view selector);
    
  private:
    // 缓存解析后的选择器以提高性能
    static std::unique_ptr<SelectorList> parse_and_cache(std::string_view selector);
};

}  // namespace hps