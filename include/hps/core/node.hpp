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

    /**
     * @brief 判断是否为 DOCTYPE 节点
     * @return 如果是 DOCTYPE 节点则返回 true
     */
    [[nodiscard]] bool is_doctype() const noexcept {
        return m_type == NodeType::Doctype;
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
        return m_first_child != nullptr;
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

    /**
     * @brief 尝试将节点转换为 DoctypeNode 类型
     * @return DoctypeNode 节点的原始指针，如果转换失败则为 nullptr
     */
    [[nodiscard]] const DoctypeNode* as_doctype() const noexcept;

  protected:
    /**
     * @brief 获取所属文档
     * @return 当前节点所在的文档，未挂载则返回 nullptr
     */
    [[nodiscard]] const Document* owner_document() const noexcept;

    /**
     * @brief 获取所属文档（可变）
     * @return 当前节点所在的文档，未挂载则返回 nullptr
     */
    [[nodiscard]] Document* owner_document_mut() noexcept;

    /**
     * @brief 通知所属文档清理查询缓存
     */
    void invalidate_document_query_cache() noexcept;

    /**
     * @brief 追加子节点（不转移所有权——节点由 Document 的 arena 拥有）
     * @param child 由同一 Document 的 arena 分配的节点
     */
    Node* append_child(Node* child);

    /**
     * @brief 在指定子节点前插入一个子节点
     * @param child 要插入的子节点（arena 拥有）
     * @param before 作为参照的现有子节点；为空时退化为追加
     * @return 实际插入后的节点指针
     */
    Node* insert_child_before(Node* child, const Node* before);

    /**
     * @brief 设置父节点
     * @param parent_node 父节点的原始指针
     */
    void set_parent(Node* parent_node) noexcept;

    /**
     * @brief 取出并移除所有子节点（仅断链，节点仍由 arena 拥有）
     * @return 原有子节点的裸指针数组（按文档序）
     */
    std::vector<Node*> take_children();

    /**
     * @brief 从子节点链中摘除指定子节点（仅断链，节点仍由 arena 拥有）
     * @param child 要摘除的子节点
     *
     * 修复兄弟链与父指针，供树重排（如领养机构算法）使用。
     */
    void remove_child(Node* child);

  private:
    // Document 在工厂中创建节点后写入本指针（友元访问）：节点的拥有文档持有其 arena
    // 与字符串池。owner_document() 因此为 O(1)，且供 add_attribute 等就地驻留字符串。
    friend class Document;

    NodeType  m_type;
    Document* m_owner_document{nullptr};  ///< 创建该节点的 Document（拥有 arena 与字符串池）
    Node*     m_parent{nullptr};
    Node*     m_first_child{nullptr};  ///< 侵入式子节点链表头（arena 拥有节点）
    Node*     m_last_child{nullptr};   ///< 侵入式子节点链表尾
    Node*     m_prev_sibling{nullptr};
    Node*     m_next_sibling{nullptr};
};

}  // namespace hps
