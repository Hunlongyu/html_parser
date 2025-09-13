#pragma once
#include "hps/core/node.hpp"

namespace hps {

class ElementQuery;

class Document : public Node {
  public:
    explicit Document(std::string html_content);
    ~Document() override = default;

    NodeType    node_type() const;
    std::string node_name() const;
    std::string node_value() const;
    std::string text_content() const override;

    std::string title() const;
    std::string charset() const;
    std::string source_html() const;

    std::string get_meta_content(std::string_view name) const;
    std::string get_meta_property(std::string_view property) const;

    std::vector<std::string> get_all_links() const;
    std::vector<std::string> get_all_images() const;

    std::shared_ptr<const Element> root() const;
    std::shared_ptr<const Element> html() const;
    std::shared_ptr<const Element> document() const;

    std::shared_ptr<const Element>              querySelector(std::string_view selector) const;
    std::vector<std::shared_ptr<const Element>> querySelectorAll(std::string_view selector) const;

    std::shared_ptr<const Element>              get_element_by_id(std::string_view id) const;
    std::vector<std::shared_ptr<const Element>> get_elements_by_tag_name(std::string_view tag_name) const;
    std::vector<std::shared_ptr<const Element>> get_elements_by_class_name(std::string_view class_name) const;

    ElementQuery css(std::string_view selector) const;
    ElementQuery xpath(std::string_view expression) const;

    void add_child(std::unique_ptr<Node> child);

  private:
    std::string m_html_source;
};
}  // namespace hps