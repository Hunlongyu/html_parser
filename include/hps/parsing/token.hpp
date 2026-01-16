#pragma once

#include "hps/hps_fwd.hpp"
#include "hps/parsing/token_attribute.hpp"
#include "hps/utils/noncopyable.hpp"

#include <vector>

namespace hps {

/**
 * @brief HTML Token 类
 *
 * 表示HTML解析过程中的一个词法单元，包含类型、名称、值和属性信息。
 * Token是词法分析器(Tokenizer)的输出，也是语法分析器(TreeBuilder)的输入。
 *
 * @note 继承自NonCopyable，禁止拷贝但允许移动，确保性能和资源安全
 */
class Token : public NonCopyable {
  public:
    // === 构造和析构函数 ===

    /**
     * @brief 构造函数
     * @param type Token类型
     * @param name Token名称（标签名或其他标识符）
     * @param value Token值（文本内容或注释内容）
     */
    Token(TokenType type, std::string_view name, std::string_view value) noexcept;

    /**
     * @brief 移动构造函数
     * @param other 要移动的Token对象
     *
     * 高效转移Token的所有权，避免不必要的拷贝开销。
     * 在tokenizer中大量使用，对性能至关重要。
     */
    Token(Token&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的Token对象
     * @return 当前对象的引用
     *
     * 高效转移Token的所有权，支持链式赋值操作。
     */
    Token& operator=(Token&& other) noexcept;

    /**
     * @brief 析构函数
     *
     * 使用默认析构函数，成员变量会自动释放资源
     */
    ~Token() = default;

    // === 核心属性访问器（最重要的基础功能）===

    /**
     * @brief 获取Token类型
     * @return Token类型枚举值
     */
    [[nodiscard]] TokenType type() const noexcept;

    /**
     * @brief 获取Token名称
     * @return Token名称的字符串视图
     */
    [[nodiscard]] std::string_view name() const noexcept;

    /**
     * @brief 获取Token值
     * @return Token值的字符串视图
     */
    [[nodiscard]] std::string_view value() const noexcept;

    // === 类型修改器 ===

    /**
     * @brief 修改Token类型
     * @param type 新的Token类型
     *
     * 用于在解析过程中动态调整Token类型，如将普通标签转换为自闭合标签
     */
    void set_type(TokenType type) noexcept;

    /**
     * @brief 设置拥有的Token值（移动语义）
     * @param value 要获取所有权的字符串
     *
     * 当Token的内容是动态生成而非直接来自源码时使用。
     * 此时Token将拥有该字符串的所有权。
     */
    void set_owned_value(std::string value);

    // === 属性管理（重要的扩展功能）===

    /**
     * @brief 添加完整属性对象
     * @param attr 完整的TokenAttribute对象
     *
     * 用于添加已构造好的属性对象，支持复杂属性处理场景
     */
    void add_attr(const TokenAttribute& attr);

    /**
     * @brief 添加完整属性对象（移动语义）
     * @param attr 完整的TokenAttribute对象（右值）
     *
     * 高效添加属性，避免拷贝。
     */
    void add_attr(TokenAttribute&& attr);

    /**
     * @brief 添加属性（便捷方法）
     * @param name 属性名
     * @param value 属性值
     * @param has_value 是否有值，用于区分 <input disabled> 和 <input disabled="true">
     *
     * 最常用的属性添加方法，直接构造TokenAttribute对象
     */
    void add_attr(std::string_view name, std::string_view value, bool has_value = true);

    /**
     * @brief 获取属性列表
     * @return 属性列表的常量引用
     *
     * 返回所有属性的只读访问，用于遍历和查询属性
     */
    [[nodiscard]] const std::vector<TokenAttribute>& attrs() const noexcept;

    // === 基础类型检查（按照Token类型的重要性排序）===

    /**
     * @brief 检查是否为开始标签类型
     * @return true: 是开始标签, false: 不是开始标签
     *
     * 最常用的类型检查，用于识别 <div>, <p> 等开始标签
     */
    [[nodiscard]] bool is_open() const noexcept;

    /**
     * @brief 检查是否为结束标签类型
     * @return true: 是结束标签, false: 不是结束标签
     *
     * 用于识别 </div>, </p> 等结束标签
     */
    [[nodiscard]] bool is_close() const noexcept;

    /**
     * @brief 检查是否为自闭合标签类型
     * @return true: 是自闭合标签, false: 不是自闭合标签
     *
     * 用于识别 <br/>, <img/> 等自闭合标签
     */
    [[nodiscard]] bool is_close_self() const noexcept;

    /**
     * @brief 检查是否为文本类型
     * @return true: 是文本, false: 不是文本
     *
     * 用于识别标签间的文本内容
     */
    [[nodiscard]] bool is_text() const noexcept;

    /**
     * @brief 检查是否为注释类型
     * @return true: 是注释, false: 不是注释
     *
     * 用于识别 <!-- comment --> 注释内容
     */
    [[nodiscard]] bool is_comment() const noexcept;

    /**
     * @brief 检查是否为DOCTYPE类型
     * @return true: 是DOCTYPE, false: 不是DOCTYPE
     *
     * 用于识别 <!DOCTYPE html> 声明
     */
    [[nodiscard]] bool is_doctype() const noexcept;

    // === 特殊状态检查 ===

    /**
     * @brief 检查是否为解析完成标记
     * @return true: 解析完成, false: 未完成
     *
     * 用于标识tokenizer解析结束，通常在解析循环中作为终止条件
     */
    [[nodiscard]] bool is_done() const noexcept;

    /**
     * @brief 检查是否为强制怪异模式类型
     * @return true: 强制怪异模式, false: 不是
     *
     * 用于HTML5解析器的怪异模式处理
     */
    [[nodiscard]] bool is_force_quirks() const noexcept;

    // === 高级功能（特定标签匹配）===

    /**
     * @brief 检查是否为指定名称的标签
     * @param name 要匹配的标签名称
     * @return true: 匹配指定标签, false: 不匹配
     *
     * 便捷方法，用于快速检查特定标签类型，如 token.is_tag("div")
     */
    [[nodiscard]] bool is_tag(std::string_view name) const noexcept;

  private:
    TokenType                   m_type;        ///< Token类型，决定了Token的基本行为
    std::string                 m_name;        ///< Token名称，对于标签是标签名，对于文本通常为空
    std::string_view            m_value;       ///< Token值，用于零拷贝场景（指向源码）
    std::string                 m_value_owned; ///< Token值，用于拥有所有权的场景（动态内容）
    std::vector<TokenAttribute> m_attrs;       ///< 属性列表，存储标签的所有属性信息
};

}  // namespace hps