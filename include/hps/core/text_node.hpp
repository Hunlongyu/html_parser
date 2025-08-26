#pragma once

#include "core/node.hpp"

namespace hps {

class TextNode : public Node {
  public:
    explicit TextNode(std::string_view text);
    ~TextNode() override = default;

    NodeType node_type() const override;

    std::string_view node_name() const override;

    std::string_view node_value() const override;

    std::string_view text_content() const override;

    std::string_view text() const;

    bool empty() const noexcept;

    size_t length() const noexcept;

  private:
    std::string_view m_text;
};

}  // namespace hps
