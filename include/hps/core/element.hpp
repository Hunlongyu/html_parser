#pragma once

#include "hps/core/attribute.hpp"
#include "hps/core/node.hpp"

namespace hps {

class ElementQuery;

class Element : public Node {
  public:
    /**
     * @brief 构造函数
     * @param name 元素的标签名
     */
    explicit Element(std::string_view name);

    /**
     * @brief 虚析构函数
     */
    ~Element() override = default;

    // Node Interface Overrides
    /**
     * @brief 获取节点类型
     * @return 节点类型，对于 Element 始终返回 NodeType::Element
     */
    [[nodiscard]] NodeType type() const noexcept override;

    /**
     * @brief 递归获取元素的所有文本内容
     * @return 元素及其所有子元素的文本内容拼接而成的字符串
     */
    [[nodiscard]] std::string text_content() const override;

    /**
     * @brief 获取元素自身的文本内容
     * @return 元素自身的文本内容
     */
    [[nodiscard]] std::string own_text_content() const;

    // Element Specific Properties
    /**
     * @brief 获取元素标签名
     * @return 元素的标签名（例如 "div", "p", "a"）。
     */
    [[nodiscard]] const std::string& tag_name() const noexcept;

    // Attribute Management
    /**
     * @brief 检查元素是否存在指定属性
     * @param name 属性名
     * @return 如果存在指定属性则返回 true，否则返回 false
     */
    [[nodiscard]] bool has_attribute(std::string_view name) const noexcept;

    /**
     * @brief 获取指定属性的值
     * @param name 属性名（忽略大小写）
     * @return 属性值，如果属性不存在则返回空字符串
     */
    [[nodiscard]] std::string get_attribute(std::string_view name) const noexcept;

    /**
     * @brief 获取所有属性
     * @return 属性列表的常量引用
     */
    [[nodiscard]] const std::vector<Attribute>& attributes() const noexcept;

    /**
     * @brief 获取属性数量
     * @return 元素的属性数量
     */
    [[nodiscard]] size_t attribute_count() const noexcept;

    /**
     * @brief 获取 ID 属性值
     * @return ID 值，如果元素没有 ID 属性则返回空字符串
     */
    [[nodiscard]] std::string id() const noexcept;

    /**
     * @brief 获取 class 属性的原始值
     * @return class 属性值，如果元素没有 class 属性则返回空字符串
     */
    [[nodiscard]] std::string class_name() const noexcept;

    /**
     * @brief 获取所有 CSS 类名
     * @return 包含所有 CSS 类名的集合的常量引用
     */
    [[nodiscard]] std::unordered_set<std::string> class_names() const noexcept;

    /**
     * @brief 检查元素是否包含指定 class 类
     * @param class_name 要检查的类名
     * @return 如果包含指定类则返回 true，否则返回 false
     */
    [[nodiscard]] bool has_class(std::string_view class_name) const noexcept;

    // Query Methods
    /**
     * @brief 使用 CSS 选择器查询第一个匹配的子元素
     * @param selector CSS 选择器字符串
     * @return 第一个匹配的子元素的共享指针，如果没有匹配则返回 nullptr
     */
    [[nodiscard]] std::shared_ptr<const Element> querySelector(std::string_view selector) const;

    /**
     * @brief 使用 CSS 选择器查询所有匹配的子元素
     * @param selector CSS 选择器字符串
     * @return 所有匹配的子元素的共享指针列表
     */
    [[nodiscard]] std::vector<std::shared_ptr<const Element>> querySelectorAll(std::string_view selector) const;

    /**
     * @brief 按 ID 获取子元素
     * @param id 要查找的 ID 值
     * @return 匹配的子元素的共享指针，如果没有匹配则返回 nullptr
     */
    [[nodiscard]] std::shared_ptr<const Element> get_element_by_id(std::string_view id) const;

    /**
     * @brief 按标签名获取直接子元素
     * @param tag_name 要查找的标签名
     * @return 匹配的直接子元素列表
     */
    [[nodiscard]] std::vector<std::shared_ptr<const Element>> get_elements_by_tag_name(std::string_view tag_name) const;

    /**
     * @brief 按类名获取直接子元素
     * @param class_name 要查找的类名
     * @return 匹配的直接子元素列表
     */
    [[nodiscard]] std::vector<std::shared_ptr<const Element>> get_elements_by_class_name(std::string_view class_name) const;

    /**
     * @brief 创建一个 ElementQuery 对象，用于链式 CSS 选择器查询
     * @param selector 初始的 CSS 选择器字符串
     * @return ElementQuery 对象
     */
    [[nodiscard]] ElementQuery css(std::string_view selector) const;

    /**
     * @brief 创建一个 ElementQuery 对象，用于链式 XPath 查询
     * @param expression XPath 表达式字符串
     * @return ElementQuery 对象
     */
    [[nodiscard]] ElementQuery xpath(std::string_view expression) const;

    // Tree Modification
    /**
     * @brief 添加子节点
     * @param child 要添加的子节点的唯一指针
     */
    void add_child(const std::shared_ptr<Node>& child);

    /**
     * @brief 添加或更新属性
     * @param name 属性名
     * @param value 属性值
     */
    void add_attribute(std::string_view name, std::string_view value);

  private:
    std::string            m_name;       /**< 标签名 */
    std::vector<Attribute> m_attributes; /**< 属性列表 */
};

}  // namespace hps
