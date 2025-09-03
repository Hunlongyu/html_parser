#pragma once

#include "hps/utils/noncopyable.hpp"

#include <functional>
#include <string>
#include <string_view>

namespace hps {

class Element;

class QueryEngine : public NonCopyable {
  public:
    QueryEngine();
    explicit QueryEngine(const Element* element);
    explicit QueryEngine(std::vector<const Element*> elements);
    ~QueryEngine();

    QueryEngine filter(std::function<bool(const Element*)> predicate) const;

    [[nodiscard]] QueryEngine has_id(std::string_view id) const;
    [[nodiscard]] QueryEngine has_tag(std::string_view tag_name) const;
    [[nodiscard]] QueryEngine has_class(std::string_view class_name) const;
    [[nodiscard]] QueryEngine has_attribute(std::string_view attribute_name) const;

    [[nodiscard]] QueryEngine tag_equals(std::string_view attribute_name,
                                         std::string_view value) const;
    [[nodiscard]] QueryEngine attribute_equals(std::string_view attribute_name,
                                               std::string_view value) const;
    [[nodiscard]] QueryEngine attribute_contains(std::string_view attribute_name,
                                                 std::string_view value) const;
    [[nodiscard]] QueryEngine text_contains(std::string_view text) const;

    [[nodiscard]] QueryEngine first() const;
    [[nodiscard]] QueryEngine last() const;
    [[nodiscard]] QueryEngine slice(size_t start, size_t count) const;
    [[nodiscard]] QueryEngine event() const;
    [[nodiscard]] QueryEngine odd() const;

    [[nodiscard]] QueryEngine find(std::string_view css_selector) const;
    [[nodiscard]] QueryEngine query(std::string_view css_selector) const;
    [[nodiscard]] QueryEngine select(std::string_view css_selector) const;

    [[nodiscard]] QueryEngine querySelector(std::string_view css_selector) const;
    [[nodiscard]] QueryEngine querySelectorAll(std::string_view css_selector) const;

    [[nodiscard]] QueryEngine children(std::string_view css_selector = {}) const;
    [[nodiscard]] QueryEngine parent() const;
    [[nodiscard]] QueryEngine parents() const;
    [[nodiscard]] QueryEngine siblings() const;
    [[nodiscard]] QueryEngine next() const;
    [[nodiscard]] QueryEngine prev() const;

    [[nodiscard]] std::vector<const Element*> to_vector() const;
    [[nodiscard]] const Element*              first_element() const;
    [[nodiscard]] const Element*              last_element() const;
    [[nodiscard]] const Element*              element_at(size_t index) const;

    [[nodiscard]] size_t size() const;
    [[nodiscard]] size_t count() const;
    [[nodiscard]] bool   empty() const;

    //[[nodiscard]] auto begin() const;
    //[[nodiscard]] auto end() const;

    [[nodiscard]] std::vector<std::string> texts() const;
    [[nodiscard]] std::vector<std::string> inner_texts() const;
    [[nodiscard]] std::vector<std::string> attributes(std::string_view attribute_name) const;
    [[nodiscard]] std::vector<std::string> hrefs() const;
    [[nodiscard]] std::vector<std::string> srcs() const;
    [[nodiscard]] std::vector<std::string> ids() const;
    [[nodiscard]] std::vector<std::string> classes() const;
    [[nodiscard]] std::vector<std::string> outer_htmls() const;
    [[nodiscard]] std::vector<std::string> inner_htmls() const;

    size_t count_by_tag(std::string_view tag_name) const;
    size_t count_by_class(std::string_view class_name) const;
    size_t count_by_attribute(std::string_view attribute_name) const;

    bool any_has_attribute(std::string_view attr_name) const;
    bool all_have_attribute(std::string_view attr_name) const;
    bool any_has_class(std::string_view class_name) const;
    bool all_have_class(std::string_view class_name) const;

  private:
    std::vector<const Element*> m_elements;
};
}  // namespace hps