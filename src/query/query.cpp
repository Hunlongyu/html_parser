#include "hps/query/query.hpp"

#include "hps/query/css/css_matcher.hpp"
#include "hps/query/css/css_utils.hpp"
#include "hps/query/element_query.hpp"
#include "hps/query/xpath/xpath_query.hpp"
namespace hps {

ElementQuery Query::css(const Element& element, const std::string_view selector) {
    const auto selector_list = parse_css_selector(selector);
    if (!selector_list) {
        return ElementQuery{};
    }
    auto results = CSSMatcher::find_all(element, *selector_list);
    return ElementQuery(std::move(results));
}

ElementQuery Query::css(const Document& document, const std::string_view selector) {
    const auto selector_list = parse_css_selector(selector);
    if (!selector_list) {
        return ElementQuery{};
    }
    auto results = CSSMatcher::find_all(document, *selector_list);
    return ElementQuery(std::move(results));
}

ElementQuery Query::xpath(const Element& element, const std::string_view expression) {
    XPathQuery query(expression);
    return query.select(element);
}

ElementQuery Query::xpath(const Document& document, const std::string_view expression) {
    XPathQuery query(expression);
    return query.select(document);
}

}  // namespace hps