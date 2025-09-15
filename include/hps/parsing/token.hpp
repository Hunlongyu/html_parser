#pragma once

#include "hps/hps_fwd.hpp"
#include "hps/parsing/token_attribute.hpp"
#include "hps/utils/noncopyable.hpp"

namespace hps {

class Token : public NonCopyable {
  public:
    Token(TokenType type, std::string_view name, std::string_view value) noexcept;
    ~Token() = default;

    Token(Token&&) noexcept            = default;
    Token& operator=(Token&&) noexcept = default;

    TokenType        type() const noexcept;
    std::string_view name() const noexcept;
    std::string_view value() const noexcept;

    /**
     * @brief 修改类型
     * @param type 类型
     */
    void set_type(TokenType type) noexcept;

    /**
     * @brief 添加属性
     * @param attr 完整属性
     */
    void add_attr(const TokenAttribute& attr);

    /**
     * @brief 添加属性
     * @param name 属性名
     * @param value 属性值
     * @param has_value 是否有值
     */
    void add_attr(std::string_view name, std::string_view value, bool has_value = true);

    /// @brief 获取属性列表
    /// @return 属性列表
    const std::vector<TokenAttribute>& attrs() const noexcept;

    // 类型检查便利方法
    bool is_open() const noexcept;
    bool is_close() const noexcept;
    bool is_close_self() const noexcept;
    bool is_force_quirks() const noexcept;
    bool is_done() const noexcept;
    bool is_text() const noexcept;
    bool is_comment() const noexcept;
    bool is_doctype() const noexcept;

    /// @brief 特定标签检查
    /// @param name 标签名称
    /// @return true: 匹配, false: 不匹配
    bool is_tag(std::string_view name) const noexcept;

  private:
    TokenType                   m_type;
    std::string                 m_name;
    std::string                 m_value;
    std::vector<TokenAttribute> m_attrs;
};

}  // namespace hps