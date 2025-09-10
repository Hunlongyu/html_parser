#pragma once
#include "hps/utils/noncopyable.hpp"

#include <memory>
#include <string>
#include <vector>

namespace hps {

enum class NodeType {
    Undefined,        /**< 未定义节点 */
    Element,          /**< 元素节点 <div> */
    Text,             /**< 文本节点 <p>Hello</p> 包括 <![CDATA[hello world]]> */
    Comment,          /**< <!-- 注释 --> */
    Document,         /**< 文档节点 整个文档的根节点 */
    DocumentType,     /**< 文档类型节点 <!DOCTYPE html> */
    DocumentFragment, /**< 文档片段节点 */
};

class Node : public NonCopyable {
  public:
    virtual ~Node() = default;

    /**
     * @brief 获取节点类型
     * @return 节点类型
     */
    [[nodiscard]] virtual NodeType node_type() const = 0;

    /**
     * @brief 获取节点名称
     * @return 节点名称
     */
    [[nodiscard]] virtual std::string node_name() const = 0;

    /**
     * @brief 获取节点内容
     * @return 节点内容
     */
    [[nodiscard]] virtual std::string node_value() const = 0;

    /**
     * @brief 递归获取节点所有文本内容
     * @return 节点文本内容
     */
    [[nodiscard]] virtual std::string text_content() const = 0;

    /**
     * @brief 获取父节点
     * @return 父节点
     */
    [[nodiscard]] const Node* parent() const noexcept;

    /**
     * @brief 获取第一个子节点
     * @return 第一个子节点
     */
    [[nodiscard]] const Node* first_child() const noexcept;

    /**
     * @brief 获取最后一个子节点
     * @return 最后一个子节点
     */
    [[nodiscard]] const Node* last_child() const noexcept;

    /**
     * @brief 获取下一个兄弟节点
     * @return 下一个兄弟节点
     */
    [[nodiscard]] const Node* next_sibling() const noexcept;

    /**
     * @brief 获取前一个兄弟节点
     * @return 前一个兄弟节点
     */
    [[nodiscard]] const Node* previous_sibling() const noexcept;

    /**
     * @brief 获取所有子节点
     * @return 所有子节点
     */
    [[nodiscard]] const std::vector<std::unique_ptr<Node>>& children() const noexcept;

    /**
     * @brief 判断是否有子节点
     * @return true:有子节点, false:无子节点
     */
    [[nodiscard]] bool has_children() const noexcept;

    /**
     * @brief 获取子节点数量
     * @return 子节点数量
     */
    [[nodiscard]] size_t child_count() const noexcept;

    /**
     * @brief 获取指定索引的子节点
     * @param index  索引
     * @return 子节点
     */
    [[nodiscard]] const Node* child_at(size_t index) const noexcept;

    /**
     * @brief 检查节点是否是 Element 类型
     * @return true: 是, false: 否
     */
    [[nodiscard]] bool is_element() const noexcept;

    /**
     * @brief 检查节点是否是 Text 类型
     * @return true: 是, false: 否
     */
    [[nodiscard]] bool is_text() const noexcept;

    /**
     * @brief 检查节点是否是 Comment 类型
     * @return true: 是, false: 否
     */
    [[nodiscard]] bool is_comment() const noexcept;

    /**
     * @brief 检查节点是否是 Document 类型
     * @return true: 是, false: 否
     */
    [[nodiscard]] bool is_document() const noexcept;

  protected:
    explicit Node(NodeType type);               /**< 构造函数 */
    explicit Node(NodeType type, Node* parent); /**< 构造函数 */

    /**
     * @brief 设置父节点
     * @param parent 父节点
     */
    void set_parent(Node* parent);

    /**
     * @brief 添加子节点
     * @param child 子节点
     */
    virtual void add_child(std::unique_ptr<Node> child);

  private:
    NodeType                           m_type;     /**< 节点类型 */
    Node*                              m_parent;   /**< 父节点 */
    std::vector<std::unique_ptr<Node>> m_children; /**< 子节点 */
};

}  // namespace hps
