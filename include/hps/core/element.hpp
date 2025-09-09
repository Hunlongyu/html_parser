#pragma once

#include "hps/core/attribute.hpp"
#include "hps/core/node.hpp"

#include <unordered_set>

namespace hps {

class ElementQuery;

class Element : public Node {
  public:
    explicit Element(std::string_view name);
    ~Element() override = default;

    /**
     * @brief 元素类型
     * @return 元素类型
     */
    NodeType node_type() const override;

    /**
     * @brief 元素名
     * @return 元素名
     */
    std::string_view node_name() const override;

    /**
     * @brief 元素值
     * @return 元素值
     */
    std::string_view node_value() const override;

    /**
     * @brief 递归获取元素的所有文本内容
     * @return 文本内容
     */
    std::string_view text_content() const override;

    /**
     * @brief 检查是否存在指定属性
     * @param name 属性名
     * @return 存在返回 true，否则返回 false
     */
    bool has_attribute(std::string_view name) const noexcept;

    /**
     * @brief 获取指定属性的值
     * @param name 属性名【忽略大小写】
     * @return 属性值，不存在返回空 string_view
     */
    std::string_view get_attribute(std::string_view name) const noexcept;

    /**
     * @brief 获取所有属性
     * @return 属性列表的常量引用
     */
    const std::vector<Attribute>& attributes() const noexcept;

    /**
     * @brief 获取属性数量
     * @return 属性数量
     */
    size_t attribute_count() const noexcept;

    /**
     * @brief 获取 ID 属性值
     * @return ID 值，无 ID 属性返回空 string_view
     */
    std::string_view id() const noexcept;

    /**
     * @brief 获取元素标签名
     * @return 标签名
     */
    std::string_view tag_name() const noexcept;

    /**
     * @brief 获取所有 CSS 类名
     * @return 类名集合
     */
    const std::unordered_set<std::string_view>& class_names() const noexcept;

    /**
     * @brief 获取 class 属性的原始值
     * @return class 属性值，无 class 属性返回空 string_view
     */
    std::string_view class_name() const noexcept;

    /**
     * @brief 检查元素是否包含指定 class 类
     * @param class_name 类名
     * @return 包含：true，不包含：false
     */
    bool has_class(std::string_view class_name) const noexcept;

    const Element* querySelector(std::string_view selector) const;
    std::vector<const Element*> querySelectorAll(std::string_view selector) const;

    /**
     * @brief 按 ID 获取元素
     * @param id ID 值
     * @return 匹配的元素，无匹配返回 nullptr
     */
    const Element* get_element_by_id(std::string_view id) const;

    /**
     * @brief 按标签名获取直接子元素
     * @param tag_name 标签名
     * @return 匹配的子元素列表
     */
    std::vector<const Element*> get_elements_by_tag_name(std::string_view tag_name) const;

    /**
     * @brief 按类名获取直接子元素
     * @param class_name 类名
     * @return 匹配的子元素列表
     */
    std::vector<const Element*> get_elements_by_class_name(std::string_view class_name) const;

    ElementQuery css(std::string_view selector) const;
    ElementQuery xpath(std::string_view expression) const;

  private:
    /**
     * @brief 添加属性【仅解析器使用】
     * @param name 属性名
     * @param value 属性值
     */
    void add_attribute(std::string_view name, std::string_view value);

  private:
    std::string            m_name;       /**< 标签名 */
    std::vector<Attribute> m_attributes; /**< 属性列表 */
};

}  // namespace hps
