#pragma once
#include "hps/hps_fwd.hpp"

#include <memory>
#include <string>
#include <vector>

namespace hps {
class Node {
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

    // 禁用拷贝构造和拷贝赋值，因为 unique_ptr 不能拷贝
    Node(const Node&)            = delete;
    Node& operator=(const Node&) = delete;

    // 禁用移动构造和移动赋值，因为 Node 对象地址变化会导致子节点的 m_parent 指针失效
    // 且 DOM 节点通常由 unique_ptr 管理，应移动指针而不是对象本身
    Node(Node&&)            = delete;
    Node& operator=(Node&&) = delete;

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
     * @return 父节点的原始指针，如果不存在则为 nullptr
     */
    [[nodiscard]] const Node* parent() const noexcept;

    /**
     * @brief 判断是否有父节点
     * @return 如果有父节点则返回 true
     */
    [[nodiscard]] bool has_parent() const noexcept {
        return m_parent != nullptr;
    }

    /**
     * @brief 获取所有子节点
     * @return 包含所有子节点原始指针的 vector
     */
    [[nodiscard]] std::vector<const Node*> children() const noexcept;

    /**
     * @brief 判断是否有子节点
     * @return 如果有子节点则返回 true
     */
    [[nodiscard]] bool has_children() const noexcept {
        return !m_children.empty();
    }

    /**
     * @brief 获取第一个子节点
     * @return 第一个子节点的原始指针，如果不存在则为 nullptr
     */
    [[nodiscard]] const Node* first_child() const noexcept;

    /**
     * @brief 获取最后一个子节点
     * @return 最后一个子节点的原始指针，如果不存在则为 nullptr
     */
    [[nodiscard]] const Node* last_child() const noexcept;

    /**
     * @brief 获取最后一个子节点（可变）
     * @return 最后一个子节点的原始指针，如果不存在则为 nullptr
     */
    [[nodiscard]] Node* last_child_mut() noexcept;

    /**
     * @brief 获取前一个兄弟节点
     * @return 前一个兄弟节点的原始指针，如果不存在则为 nullptr
     */
    [[nodiscard]] const Node* previous_sibling() const noexcept;

    /**
     * @brief 获取后一个兄弟节点
     * @return 后一个兄弟节点的原始指针，如果不存在则为 nullptr
     */
    [[nodiscard]] const Node* next_sibling() const noexcept;

    /**
     * @brief 获取所有兄弟节点
     * @return 包含所有兄弟节点的 vector，不包括当前节点自身
     */
    [[nodiscard]] std::vector<const Node*> siblings() const noexcept;

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
     * @return Document 节点的原始指针，如果转换失败则为 nullptr
     */
    [[nodiscard]] const Document* as_document() const noexcept;

    /**
     * @brief 尝试将节点转换为 Element 类型
     * @return Element 节点的原始指针，如果转换失败则为 nullptr
     */
    [[nodiscard]] const Element* as_element() const noexcept;

    /**
     * @brief 尝试将节点转换为 TextNode 类型
     * @return TextNode 节点的原始指针，如果转换失败则为 nullptr
     */
    [[nodiscard]] const TextNode* as_text() const noexcept;

    /**
     * @brief 尝试将节点转换为 CommentNode 类型
     * @return CommentNode 节点的原始指针，如果转换失败则为 nullptr
     */
    [[nodiscard]] const CommentNode* as_comment() const noexcept;

  protected:
    /**
     * @brief 添加子节点
     * @param child 子节点的唯一指针（所有权转移）
     */
    void append_child(std::unique_ptr<Node> child);

    /**
     * @brief 设置父节点
     * @param parent_node 父节点的原始指针
     */
    void set_parent(Node* parent_node) noexcept;

  private:
    NodeType                           m_type;
    Node*                              m_parent{nullptr};
    std::vector<std::unique_ptr<Node>> m_children;
    Node*                              m_prev_sibling{nullptr};
    Node*                              m_next_sibling{nullptr};
};

}  // namespace hps
