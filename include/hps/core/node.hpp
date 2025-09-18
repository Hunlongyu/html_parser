#pragma once
#include "hps/hps_fwd.hpp"

#include <memory>

namespace hps {
class Node : public std::enable_shared_from_this<Node> {
  public:
    /**
     * @brief 构造函数
     * @param type 节点类型
     */
    explicit Node(NodeType type) noexcept;

    /**
     * @brief 虚析构函数
     */
    virtual ~Node() = default;

    /**
     * @brief 获取节点类型
     * @return 节点类型
     */
    [[nodiscard]] virtual NodeType type() const noexcept {
        return m_type;
    }

    /**
     * @brief 判断是否为 Document 节点
     * @return 如果是 Document 节点则返回 true
     */
    [[nodiscard]] bool is_document() const noexcept {
        return m_type == NodeType::Document;
    }

    /**
     * @brief 判断是否为 Element 节点
     * @return 如果是 Element 节点则返回 true
     */
    [[nodiscard]] bool is_element() const noexcept {
        return m_type == NodeType::Element;
    }

    /**
     * @brief 判断是否为 Text 节点
     * @return 如果是 Text 节点则返回 true
     */
    [[nodiscard]] bool is_text() const noexcept {
        return m_type == NodeType::Text;
    }

    /**
     * @brief 判断是否为 Comment 节点
     * @return 如果是 Comment 节点则返回 true
     */
    [[nodiscard]] bool is_comment() const noexcept {
        return m_type == NodeType::Comment;
    }

    // Tree traversal
    /**
     * @brief 获取父节点
     * @return 父节点的共享指针，如果不存在则为 nullptr
     */
    [[nodiscard]] std::shared_ptr<const Node> parent() const noexcept;

    /**
     * @brief 判断是否有父节点
     * @return 如果有父节点则返回 true
     */
    [[nodiscard]] bool has_parent() const noexcept {
        return !m_parent.expired();
    }

    /**
     * @brief 获取所有子节点
     * @return 包含所有子节点共享指针的 vector
     */
    [[nodiscard]] std::vector<std::shared_ptr<const Node>> children() const noexcept;

    /**
     * @brief 判断是否有子节点
     * @return 如果有子节点则返回 true
     */
    [[nodiscard]] bool has_children() const noexcept {
        return !m_children.empty();
    }

    /**
     * @brief 获取第一个子节点
     * @return 第一个子节点的共享指针，如果不存在则为 nullptr
     */
    [[nodiscard]] std::shared_ptr<const Node> first_child() const noexcept;

    /**
     * @brief 获取最后一个子节点
     * @return 最后一个子节点的共享指针，如果不存在则为 nullptr
     */
    [[nodiscard]] std::shared_ptr<const Node> last_child() const noexcept;

    /**
     * @brief 获取前一个兄弟节点
     * @return 前一个兄弟节点的共享指针，如果不存在则为 nullptr
     */
    [[nodiscard]] std::shared_ptr<const Node> previous_sibling() const noexcept;

    /**
     * @brief 获取后一个兄弟节点
     * @return 后一个兄弟节点的共享指针，如果不存在则为 nullptr
     */
    [[nodiscard]] std::shared_ptr<const Node> next_sibling() const noexcept;

    // Content
    /**
     * @brief 获取节点的文本内容 (虚函数)
     * @return 节点的文本内容
     */
    [[nodiscard]] virtual std::string text_content() const {
        return "";
    }

    /**
     * @brief 尝试将节点转换为 Document 类型
     * @return Document 节点的共享指针，如果转换失败则为 nullptr
     */
    [[nodiscard]] std::shared_ptr<const Document> as_document() const noexcept;

    /**
     * @brief 尝试将节点转换为 Element 类型
     * @return Element 节点的共享指针，如果转换失败则为 nullptr
     */
    [[nodiscard]] std::shared_ptr<const Element> as_element() const noexcept;

    /**
     * @brief 尝试将节点转换为 TextNode 类型
     * @return TextNode 节点的共享指针，如果转换失败则为 nullptr
     */
    [[nodiscard]] std::shared_ptr<const TextNode> as_text() const noexcept;

    /**
     * @brief 尝试将节点转换为 CommentNode 类型
     * @return CommentNode 节点的共享指针，如果转换失败则为 nullptr
     */
    [[nodiscard]] std::shared_ptr<const CommentNode> as_comment() const noexcept;

  protected:
    /**
     * @brief 添加子节点
     * @param child 子节点的共享指针
     */
    void append_child(const std::shared_ptr<Node>& child);

    /**
     * @brief 移除指定子节点
     * @param child 要移除的子节点的原始指针
     */
    void remove_child(const Node* child);

    /**
     * @brief 设置父节点
     * @param parent_node 父节点的共享指针
     */
    void set_parent(const std::shared_ptr<Node>& parent_node) noexcept;

    /**
     * @brief 从父节点中移除自身
     */
    void remove_from_parent() const noexcept;

  private:
    NodeType m_type;
    std::weak_ptr<Node> m_parent;
    std::vector<std::shared_ptr<Node>> m_children;
};

}  // namespace hps
