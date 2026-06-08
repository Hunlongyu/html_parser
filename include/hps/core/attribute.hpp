#pragma once

#include "hps/hps_fwd.hpp"

#include <string>
#include <string_view>

namespace hps {

/**
 * @brief HTML 属性（轻量值类型）
 *
 * 持有：整数化属性名 id（Attr，已知属性 → 整数比较）、属性名与属性值的 string_view
 * （指向 Document 拥有的源码/字符串池，与文档同生命周期），以及“是否带值”标志。
 * 属性本身不拥有字符串——驻留由 Element::add_attribute 经其 owner_document 完成。
 */
class Attribute {
  public:
    constexpr Attribute() noexcept = default;

    /**
     * @brief 构造（名/值须已由调用方驻留为稳定视图）
     * @param id 整数化属性名 id（未知/自定义为 Attr::Unknown）
     * @param name 属性名视图（驻留后稳定）
     * @param value 属性值视图（驻留后稳定）
     * @param has_value 是否带值（区分 disabled 与 disabled=""）
     */
    Attribute(const Attr id, const std::string_view name, const std::string_view value, const bool has_value) noexcept
        : m_id(id),
          m_name(name),
          m_value(value),
          m_has_value(has_value) {}

    /** @brief 整数化属性名 id（已知属性用于 O(1) 整数比较；自定义为 Attr::Unknown） */
    [[nodiscard]] Attr id() const noexcept {
        return m_id;
    }

    /** @brief 属性名（视图，文档存活期间有效） */
    [[nodiscard]] std::string_view name() const noexcept {
        return m_name;
    }

    /** @brief 属性值（视图，文档存活期间有效） */
    [[nodiscard]] std::string_view value() const noexcept {
        return m_value;
    }

    /** @brief 是否带值（false 表示无值属性，如 disabled） */
    [[nodiscard]] bool has_value() const noexcept {
        return m_has_value;
    }

    /**
     * @brief 字符串化：带值返回 `name="value"`；无值返回 `name`。
     */
    [[nodiscard]] std::string to_string() const {
        if (!m_has_value) {
            return std::string(m_name);
        }
        std::string out;
        out.reserve(m_name.size() + m_value.size() + 3);
        out.append(m_name).append("=\"").append(m_value).push_back('"');
        return out;
    }

    /**
     * @brief 设置属性值（value 须为调用方已驻留的稳定视图）
     */
    void set_value(const std::string_view value, const bool has_value = true) noexcept {
        m_value     = value;
        m_has_value = has_value;
    }

    auto operator<=>(const Attribute& other) const = default;

  private:
    Attr             m_id = Attr::Unknown;  /**< 整数化属性名 id */
    std::string_view m_name;                /**< 属性名（驻留视图） */
    std::string_view m_value;               /**< 属性值（驻留视图） */
    bool             m_has_value = false;   /**< 是否带值 */
};

}  // namespace hps
