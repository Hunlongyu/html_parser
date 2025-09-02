#pragma once
#include "element.hpp"
#include "node.hpp"

#include <string>

namespace hps {
class Document : public Node {
  public:
    explicit Document(std::string html_content);
    ~Document() override = default;

    NodeType         node_type() const override;
    std::string_view node_name() const override;
    std::string_view node_value() const override;
    std::string_view text_content() const override;

    const Element* find(std::string_view selector) const;
    const Element* query(std::string_view selector) const;
    const Element* select(std::string_view selector) const;

    std::vector<const Element*> find_all(std::string_view selector) const;
    std::vector<const Element*> query_all(std::string_view selector) const;
    std::vector<const Element*> select_all(std::string_view selector) const;

    const Element*              get_element_by_id(std::string_view id) const;
    std::vector<const Element*> get_elements_by_tag_name(std::string_view tag_name) const;
    std::vector<const Element*> get_elements_by_class_name(std::string_view class_name) const;

    const Element* root() const;
    const Element* html() const;
    const Element* document() const;

    std::string_view title() const;
    std::string_view charset() const;
    std::string_view source_html() const;

    std::string get_meta_content(std::string_view name) const;
    std::string get_meta_property(std::string_view property) const;

    std::vector<std::string> get_all_links() const;
    std::vector<std::string> get_all_images() const;
    std::vector<std::string> get_all_styles() const;
    std::vector<std::string> get_all_scripts() const;

  private:
    std::string m_html_source;
};

}  // namespace hps