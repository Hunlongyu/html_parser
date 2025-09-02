#include "hps/query/query_engine.hpp"

#include "hps/core/element.hpp"

#include <utility>

namespace hps {
QueryEngine::QueryEngine() {}

QueryEngine::QueryEngine(const Element* element) {}

QueryEngine::QueryEngine(std::vector<const Element*> elements) : m_elements(std::move(elements)) {}

QueryEngine QueryEngine::filter(std::function<bool(const Element*)> predicate) const {
    return {};
}

QueryEngine QueryEngine::has_id(std::string_view id) const {
    return {};
}

QueryEngine QueryEngine::has_tag(std::string_view tag_name) const {
    return {};
}

QueryEngine QueryEngine::has_class(std::string_view class_name) const {
    return {};
}

QueryEngine QueryEngine::has_attribute(std::string_view attribute_name) const {
    return {};
}

QueryEngine QueryEngine::tag_equals(std::string_view attribute_name, std::string_view value) const {
    return {};
}

QueryEngine QueryEngine::attribute_equals(std::string_view attribute_name,
                                          std::string_view value) const {
    return {};
}

QueryEngine QueryEngine::attribute_contains(std::string_view attribute_name,
                                            std::string_view value) const {
    return {};
}

QueryEngine QueryEngine::text_contains(std::string_view text) const {
    return {};
}

QueryEngine QueryEngine::first() const {
    return {};
}

QueryEngine QueryEngine::last() const {
    return {};
}

QueryEngine QueryEngine::slice(size_t start, size_t count) const {
    return {};
}

QueryEngine QueryEngine::find(std::string_view css_selector) const {
    return {};
}

QueryEngine QueryEngine::query(std::string_view css_selector) const {
    return {};
}

QueryEngine QueryEngine::select(std::string_view css_selector) const {
    return {};
}

QueryEngine QueryEngine::children(std::string_view css_selector) const {
    return {};
}

std::vector<const Element*> QueryEngine::to_vector() const {
    return {};
}

const Element* QueryEngine::first_element() const {
    return nullptr;
}

const Element* QueryEngine::last_element() const {
    return nullptr;
}

const Element* QueryEngine::element_at(size_t index) const {
    return nullptr;
}

size_t QueryEngine::size() const {
    return 0;
}

size_t QueryEngine::count() const {
    return 0;
}

bool QueryEngine::empty() const {
    return false;
}

// auto QueryEngine::begin() const {
//     return {};
// }
//
// auto QueryEngine::end() const {
//     return {};
// }

std::vector<std::string> QueryEngine::texts() const {
    return {};
}

std::vector<std::string> QueryEngine::attributes(std::string_view attribute_name) const {
    return {};
}
}  // namespace hps