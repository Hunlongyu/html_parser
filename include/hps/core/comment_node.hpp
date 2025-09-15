#pragma once

#include "hps/core/node.hpp"

namespace hps {

class CommentNode : public Node {
  public:
    explicit CommentNode(std::string_view comment) noexcept;
    ~CommentNode() override = default;

    /**
     * @brief 节点类型
     * @return 节点类型
     */
    [[nodiscard]] static NodeType type();

    /**
     * @brief 注释内容
     * @return 注释内容
     */
    [[nodiscard]] std::string value() const;

    /**
     * @brief 递归所有的注释内容
     * @return 注释内容
     */
    [[nodiscard]] std::string text_content() const override;

    /**
     * @brief 获取注释内容
     * @return 注释内容
     */
    [[nodiscard]] std::string comment() const;

    /**
     * @brief 获取注释内容 移除多余空白字符
     * @return 注释内容
     */
    [[nodiscard]] std::string normalized_comment() const;

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
    std::string m_comment;
};

}  // namespace hps
