#pragma once

#include "hps/core/node.hpp"

#include <string>
#include <string_view>

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
     * @return 该文本节点内容的视图，文档存活期间有效
     *
     * 默认零拷贝：内容是指向 Document 字符串池/源码的视图（未发生合并时）。一旦有相邻
     * 文本 token 追加合并，则物化为节点自有的可增长 std::string。两种情形对外都暴露
     * string_view。
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
    // 零拷贝默认：m_view 指向池/源码;一旦 append 合并则物化到 m_owned。
    std::string_view m_view;                 /**< 内容视图（未物化时有效） */
    std::string      m_owned;                /**< 合并后物化的可增长缓冲 */
    bool             m_materialized = false; /**< true 表示已落为 m_owned */
};

}  // namespace hps
