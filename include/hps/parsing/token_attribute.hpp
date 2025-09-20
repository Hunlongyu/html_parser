#pragma once
#include <string>

namespace hps {

/**
 * @brief Token属性结构体
 *
 * TokenAttribute表示HTML标签中的一个属性，包含属性名、属性值和是否有值的标志。
 * 这个结构体用于在词法分析阶段存储属性信息，后续会被转换为DOM树中的Attribute对象。
 *
 * 支持两种类型的属性：
 * - 有值属性：如 id="value", class="example"
 * - 无值属性：如 disabled, checked, selected
 */
struct TokenAttribute {
    std::string name;              ///< 属性名称
    std::string value;             ///< 属性值
    bool        has_value = true;  ///< 是否有值标志，区分 <input disabled> 和 <input disabled="true">

    /**
     * @brief 默认构造函数
     *
     * 创建一个空的属性对象，属性名和值都为空字符串，has_value为true。
     */
    TokenAttribute() = default;

    /**
     * @brief 参数化构造函数
     * @param n 属性名
     * @param v 属性值，默认为空字符串
     * @param hv 是否有值标志，默认为true
     *
     * 创建一个具有指定名称和值的属性。
     * 对于无值属性（如disabled），应将hv设置为false。
     *
     * 示例：
     * - TokenAttribute("id", "myId") - 有值属性
     * - TokenAttribute("disabled", "", false) - 无值属性
     */
    explicit TokenAttribute(const std::string_view n, const std::string_view v = {}, const bool hv = true)
        : name(n),
          value(v),
          has_value(hv) {}
};

}  // namespace hps