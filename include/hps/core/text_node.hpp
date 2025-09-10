#pragma once

#include "hps/core/node.hpp"

#include <string>

namespace hps {

class TextNode : public Node {
  public:
    explicit TextNode(std::string_view text) noexcept;
    ~TextNode() override = default;

    /**
     * @brief 节点类型
     * @return 节点类型
     */
    [[nodiscard]] NodeType node_type() const override;

    /**
     * @brief 节点名称
     * @return 节点名称
     */
    [[nodiscard]] std::string node_name() const override;

    /**
     * @brief 文本内容
     * @return 文本内容
     */
    [[nodiscard]] std::string node_value() const override;

    /**
     * @brief 文本内容
     * @return 文本内容
     */
    [[nodiscard]] std::string text_content() const override;

    /**
     * @brief 获取文本内容
     * @return 文本内容
     */
    [[nodiscard]] std::string text() const;

    /**
     * @brief 获取文本内容 移除多余空白字符
     * @return 文本内容
     */
    [[nodiscard]] std::string normalized_text() const;

    /**
     * @brief 判断是否为空
     * @return 是否为空
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief 获取长度
     * @return 长度
     */
    [[nodiscard]] size_t length() const noexcept;

  private:
    std::string m_text;
};

}  // namespace hps
