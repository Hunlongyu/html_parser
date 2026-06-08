#pragma once

#include "hps/core/node.hpp"

namespace hps {

class TextNode : public Node {
  public:
    explicit TextNode(std::string_view text) noexcept;
    ~TextNode() override = default;

    /**
     * @brief 节点类型
     * @return 节点类型
     */
    [[nodiscard]] NodeType type() const noexcept override;

    /**
     * @brief 获取文本内容（视图）
     * @return 该文本节点内容的视图（指向节点自有缓冲，节点存活期间有效）
     *
     * 文本节点的存储仍为可增长的 std::string（相邻文本 token 需就地追加合并），但对外
     * 统一暴露 string_view，与其它节点保持一致。
     */
    [[nodiscard]] std::string_view value() const noexcept;

    /**
     * @brief 获取文本内容（Node 多态接口；对文本节点即其自身内容）
     * @return 文本内容
     */
    [[nodiscard]] std::string text_content() const override;

    /**
     * @brief 获取文本内容 移除两端空白字符
     * @return 文本内容
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

    /**
     * @brief 追加文本内容
     * @param text 要追加的文本
     */
    void append_text(std::string_view text);

  private:
    std::string m_text;
};

}  // namespace hps
