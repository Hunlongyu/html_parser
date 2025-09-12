#pragma once
#include "hps/core/node.hpp"

namespace hps {

class ElementQuery;

class Document : public Node {
  public:
    explicit Document(std::string html_content);
    ~Document() override = default;

    NodeType         node_type() const override;
    std::string node_name() const override;
    std::string node_value() const override;
    std::string text_content() const override;

    std::string title() const;
    std::string charset() const;
    std::string source_html() const;

    std::string get_meta_content(std::string_view name) const;
    std::string get_meta_property(std::string_view property) const;

    std::vector<std::string> get_all_links() const;
    std::vector<std::string> get_all_images() const;

    const Element* root() const;
    const Element* html() const;
    const Element* document() const;

    const Element*              querySelector(std::string_view selector) const;
    std::vector<const Element*> querySelectorAll(std::string_view selector) const;

    const Element*              get_element_by_id(std::string_view id) const;
    std::vector<const Element*> get_elements_by_tag_name(std::string_view tag_name) const;
    std::vector<const Element*> get_elements_by_class_name(std::string_view class_name) const;

    ElementQuery css(std::string_view selector) const;
    ElementQuery xpath(std::string_view expression) const;

    void add_child(std::unique_ptr<Node> child) override;

  private:
    std::string m_html_source;

};
}  // namespace hps