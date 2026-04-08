#include "hps/query/query.hpp"

#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/query/css/css_matcher.hpp"
#include "hps/query/css/css_utils.hpp"
#include "hps/query/element_query.hpp"

namespace {

[[nodiscard]] std::vector<const hps::Element*> fast_path_query_all(const hps::Element& element, const hps::SimpleSelectorView selector) {
    switch (selector.kind) {
        case hps::SimpleSelectorKind::Id: {
            if (const auto* found = element.get_element_by_id(selector.value)) {
                return {found};
            }
            return {};
        }
        case hps::SimpleSelectorKind::Class:
            return element.get_elements_by_class_name(selector.value);
        case hps::SimpleSelectorKind::Type:
            return element.get_elements_by_tag_name(selector.value);
    }

    return {};
}

[[nodiscard]] std::vector<const hps::Element*> fast_path_query_all(const hps::Document& document, const hps::SimpleSelectorView selector) {
    switch (selector.kind) {
        case hps::SimpleSelectorKind::Id: {
            if (const auto* found = document.get_element_by_id(selector.value)) {
                return {found};
            }
            return {};
        }
        case hps::SimpleSelectorKind::Class:
            return document.get_elements_by_class_name(selector.value);
        case hps::SimpleSelectorKind::Type:
            return document.get_elements_by_tag_name(selector.value);
    }

    return {};
}

[[nodiscard]] const hps::Element* fast_path_query_first(const hps::Element& element, const hps::SimpleSelectorView selector) {
    switch (selector.kind) {
        case hps::SimpleSelectorKind::Id:
            return element.get_element_by_id(selector.value);
        case hps::SimpleSelectorKind::Class: {
            const auto results = element.get_elements_by_class_name(selector.value);
            return results.empty() ? nullptr : results.front();
        }
        case hps::SimpleSelectorKind::Type: {
            const auto results = element.get_elements_by_tag_name(selector.value);
            return results.empty() ? nullptr : results.front();
        }
    }

    return nullptr;
}

[[nodiscard]] const hps::Element* fast_path_query_first(const hps::Document& document, const hps::SimpleSelectorView selector) {
    switch (selector.kind) {
        case hps::SimpleSelectorKind::Id:
            return document.get_element_by_id(selector.value);
        case hps::SimpleSelectorKind::Class: {
            const auto results = document.get_elements_by_class_name(selector.value);
            return results.empty() ? nullptr : results.front();
        }
        case hps::SimpleSelectorKind::Type: {
            const auto results = document.get_elements_by_tag_name(selector.value);
            return results.empty() ? nullptr : results.front();
        }
    }

    return nullptr;
}

}  // namespace

namespace hps {

ElementQuery Query::css(const Element& element, const std::string_view selector) {
    if (const auto simple_selector = classify_simple_selector(selector); simple_selector.has_value()) {
        return ElementQuery(fast_path_query_all(element, *simple_selector));
    }

    const auto selector_list = parse_css_selector_cached(selector);
    if (!selector_list || selector_list->empty()) {
        return ElementQuery{};
    }

    auto results = CSSMatcher::find_all(element, *selector_list);
    return ElementQuery(std::move(results));
}

ElementQuery Query::css(const Element& element, const SelectorList& selector_list) {
    auto results = CSSMatcher::find_all(element, selector_list);
    return ElementQuery(std::move(results));
}

ElementQuery Query::css(const Document& document, const std::string_view selector) {
    if (const auto simple_selector = classify_simple_selector(selector); simple_selector.has_value()) {
        return ElementQuery(fast_path_query_all(document, *simple_selector));
    }

    const auto selector_list = parse_css_selector_cached(selector);
    if (!selector_list || selector_list->empty()) {
        return ElementQuery{};
    }

    auto results = CSSMatcher::find_all(document, *selector_list);
    return ElementQuery(std::move(results));
}

ElementQuery Query::css(const Document& document, const SelectorList& selector_list) {
    auto results = CSSMatcher::find_all(document, selector_list);
    return ElementQuery(std::move(results));
}

const Element* Query::css_first(const Element& element, const std::string_view selector) {
    if (const auto simple_selector = classify_simple_selector(selector); simple_selector.has_value()) {
        return fast_path_query_first(element, *simple_selector);
    }

    const auto selector_list = parse_css_selector_cached(selector);
    if (!selector_list || selector_list->empty()) {
        return nullptr;
    }

    return CSSMatcher::find_first(element, *selector_list);
}

const Element* Query::css_first(const Element& element, const SelectorList& selector_list) {
    return CSSMatcher::find_first(element, selector_list);
}

const Element* Query::css_first(const Document& document, const std::string_view selector) {
    if (const auto simple_selector = classify_simple_selector(selector); simple_selector.has_value()) {
        return fast_path_query_first(document, *simple_selector);
    }

    const auto selector_list = parse_css_selector_cached(selector);
    if (!selector_list || selector_list->empty()) {
        return nullptr;
    }

    return CSSMatcher::find_first(document, *selector_list);
}

const Element* Query::css_first(const Document& document, const SelectorList& selector_list) {
    return CSSMatcher::find_first(document, selector_list);
}

}  // namespace hps
