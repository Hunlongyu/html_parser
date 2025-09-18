#pragma once
#include "token_attribute.hpp"

#include <vector>

namespace hps {

/**
 * @brief Token构建器结构体
 *
 * TokenBuilder用于在词法分析过程中逐步构建Token对象。它维护当前正在解析的
 * 标签信息，包括标签名、属性和各种状态标志。这个结构体作为Tokenizer的
 * 内部工具，帮助管理复杂的HTML标签解析状态。
 */
struct TokenBuilder {
    // === 核心标签信息 ===
    std::string tag_name;  ///< 当前标签名称

    // === 属性构建状态 ===
    std::string attr_name;   ///< 当前正在构建的属性名
    std::string attr_value;  ///< 当前正在构建的属性值

    // === 标签类型标志 ===
    bool is_void_element = false;  ///< 是否为空元素（如<br>, <img>等）
    bool is_self_closing = false;  ///< 是否为自闭合标签（如<tag />）

    // === 属性集合 ===
    std::vector<TokenAttribute> attrs;  ///< 已完成的属性列表

    // === 属性管理方法 ===

    /**
     * @brief 添加完整属性
     * @param attr 要添加的属性对象
     *
     * 将一个完整的TokenAttribute对象添加到属性列表中。
     */
    void add_attr(const TokenAttribute& attr) {
        attrs.push_back(attr);
    }

    /**
     * @brief 添加属性（通过参数构建）
     * @param name 属性名
     * @param value 属性值
     * @param has_value 是否有值，默认为true
     *
     * 通过提供的参数直接构建并添加属性到列表中。
     * 对于无值属性（如disabled），应将has_value设为false。
     */
    void add_attr(std::string_view name, std::string_view value, bool has_value = true) {
        attrs.emplace_back(name, value, has_value);
    }

    // === 状态管理方法 ===

    /**
     * @brief 重置构建器状态
     *
     * 清空所有内部状态，准备构建下一个Token。
     * 包括标签名、属性信息、标志位和属性列表。
     */
    void reset() {
        tag_name.clear();
        attr_name.clear();
        attr_value.clear();
        is_void_element = false;
        is_self_closing = false;
        attrs.clear();
    }

    /**
     * @brief 完成当前属性的构建
     *
     * 将当前正在构建的属性（attr_name和attr_value）添加到属性列表中，
     * 然后清空临时属性构建状态。如果当前没有正在构建的属性则不执行任何操作。
     */
    void finish_current_attribute() {
        if (!attr_name.empty()) {
            add_attr(attr_name, attr_value);
            attr_name.clear();
            attr_value.clear();
        }
    }
};

}  // namespace hps