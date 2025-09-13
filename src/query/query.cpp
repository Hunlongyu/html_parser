#include "hps/query/query.hpp"
#include "hps/query/element_query.hpp"
#include "hps/query/css/css_engine.hpp"
#include "hps/core/document.hpp"
#include "hps/core/element.hpp"

namespace hps {

ElementQuery Query::css(const Element& element, std::string_view selector) {
    return CSSEngine::query(element, selector);
}

ElementQuery Query::css(const Document& document, std::string_view selector) {
    return CSSEngine::query(document, selector);
}

ElementQuery Query::xpath(const Element& element, std::string_view expression) {
    // XPath功能暂未实现
    return ElementQuery{};
}

ElementQuery Query::xpath(const Document& document, std::string_view expression) {
    // XPath功能暂未实现
    return ElementQuery{};
}

}  // namespace hps