#pragma once

#include "hps/core/node.hpp"

#include <string>
#include <string_view>

namespace hps {

class CommentNode : public Node {
  public:
    explicit CommentNode(std::string_view comment);
    ~CommentNode() override = default;

    /**
     * @brief 节点类型
     * @return 节点类型
     */
    [[nodiscard]] NodeType type() const noexcept override;

    /**
     * @brief 注释内容（视图，指向文档字符串池/源码，文档存活期间有效）
     * @return 注释内容视图
     */
    [[nodiscard]] std::string_view value() const noexcept;

    /**
     * @brief 面向文本提取时的内容
     * @return 空字符串，避免注释污染父节点的 text_content
     */
    [[nodiscard]] std::string text_content() const override;

    /**
     * @brief 获取注释内容 移除两端空白字符
     * @return 注释内容
     */
    [[nodiscard]] std::string trim() const;

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
    std::string_view m_comment;  /**< 注释内容视图（驻留：文档池/源码） */
};

}  // namespace hps
