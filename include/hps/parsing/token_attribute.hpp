#pragma once
#include <string>
#include <string_view>

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
    std::string_view value;             ///< 属性值
    bool        has_value = true;  ///< 是否有值标志，区分 <input disabled> 和 <input disabled="true">

    /**
     * @brief 默认构造函数
     *
     * 创建一个空的属性对象，属性名和值都为空字符串，has_value为true。
     */
    TokenAttribute() = default;

    /**
     * @brief 构造函数（按值传递优化）
     * @param n 属性名（支持左值拷贝或右值移动）
     * @param v 属性值，默认为空字符串
     * @param hv 是否有值标志，默认为true
     *
     * 使用按值传递（Pass-by-Value）惯用语，结合 std::move，
     * 既能处理左值也能处理右值，同时避免了重载带来的歧义。
     * 字符串字面量（const char*）会隐式转换为 std::string。
     */
    explicit TokenAttribute(std::string n, const std::string_view v = {}, const bool hv = true)
        : name(std::move(n)),
          value(v),
          has_value(hv) {}
};

}  // namespace hps