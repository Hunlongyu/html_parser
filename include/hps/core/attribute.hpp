#pragma once

#include "hps/parsing/token_attribute.hpp"

#include <string>

namespace hps {

/**
 * @brief HTML 属性类
 *
 * Attribute 类表示 HTML 元素的一个属性，包含属性名、属性值以及是否有值的标志。
 * 支持无值属性（如 disabled、checked）和有值属性（如 id="value"、class="value"）。
 * 该类提供了属性的创建、访问、修改和字符串化功能。
 */
class Attribute {
  public:
    /**
     * @brief 默认构造函数
     *
     * 创建一个空的属性对象，属性名和值都为空字符串，has_value 为 false。
     */
    constexpr Attribute() noexcept = default;

    /**
     * @brief 构造函数
     * @param name 属性名
     * @param value 属性值，默认为空字符串
     * @param hv 是否有值标志，默认为 true
     *
     * 创建一个具有指定名称和值的属性。
     * 对于无值属性（如 disabled），应将 hv 设置为 false。
     */
    explicit Attribute(const std::string_view name, const std::string_view value = {}, const bool hv = true) noexcept
        : m_name(name),
          m_value(value),
          m_has_value(hv) {}

    /**
     * @brief 从 TokenAttribute 构造
     * @param attr TokenAttribute 对象
     *
     * 从解析器生成的 TokenAttribute 对象创建 Attribute 实例。
     * 用于将解析阶段的属性信息转换为 DOM 树中的属性对象。
     */
    explicit Attribute(const TokenAttribute& attr)
        : m_name(attr.m_name),
          m_value(attr.m_value),
          m_has_value(attr.m_has_value) {}

    // Attribute Access Methods
    /**
     * @brief 获取属性名
     * @return 属性名的常量引用
     */
    [[nodiscard]] const std::string& name() const noexcept {
        return m_name;
    }

    /**
     * @brief 获取属性值
     * @return 属性值的常量引用
     */
    [[nodiscard]] const std::string& value() const noexcept {
        return m_value;
    }

    /**
     * @brief 检查属性是否有值
     * @return 如果属性有值则返回 true，否则返回 false
     *
     * 用于区分有值属性（如 id="value"）和无值属性（如 disabled）。
     */
    [[nodiscard]] bool has_value() const noexcept {
        return m_has_value;
    }

    // String Representation
    /**
     * @brief 将属性转换为字符串表示
     * @return 属性的字符串表示形式
     *
     * 对于有值属性，返回格式为 'name="value"'；
     * 对于无值属性，仅返回属性名。
     * 例如：id="header" 或 disabled
     */
    [[nodiscard]] std::string to_string() const {
        if (!m_has_value) {
            return m_name;
        }
        return m_name + "=\"" + m_value + "\"";
    }

    // Attribute Modification Methods
    /**
     * @brief 设置属性名（string 版本）
     * @param name 新的属性名
     */
    void set_name(const std::string& name) noexcept {
        m_name = name;
    }

    /**
     * @brief 设置属性名（string_view 版本）
     * @param name 新的属性名
     */
    void set_name(const std::string_view name) noexcept {
        m_name = name;
    }

    /**
     * @brief 设置属性值（string 版本）
     * @param value 新的属性值
     * @param has_value 是否有值标志，默认为 true
     *
     * 设置属性的值和有值标志。可以用于将有值属性转换为无值属性，
     * 或者将无值属性转换为有值属性。
     */
    void set_value(const std::string& value, const bool has_value = true) noexcept {
        m_value     = value;
        m_has_value = has_value;
    }

    /**
     * @brief 设置属性值（string_view 版本）
     * @param value 新的属性值
     * @param has_value 是否有值标志，默认为 true
     */
    void set_value(const std::string_view value, const bool has_value = true) noexcept {
        m_value     = value;
        m_has_value = has_value;
    }

    // Comparison Operators
    /**
     * @brief 三路比较操作符
     * @param other 要比较的另一个 Attribute 对象
     * @return 比较结果
     *
     * 提供完整的比较功能，支持 ==, !=, <, <=, >, >= 操作符。
     * 比较顺序：首先比较属性名，然后比较属性值，最后比较 has_value 标志。
     */
    auto operator<=>(const Attribute& other) const = default;

  private:
    std::string m_name;              /**< 属性名 */
    std::string m_value;             /**< 属性值 */
    bool        m_has_value = false; /**< 是否有值标志，false 表示无值属性（如 disabled） */
};

}  // namespace hps