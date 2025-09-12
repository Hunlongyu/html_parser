#pragma once
#include "hps/parsing/options.hpp"
#include "hps/parsing/token.hpp"
#include "hps/utils/exception.hpp"

#include <iostream>
#include <optional>

namespace hps {

enum class TokenizerState {
    Data,        /// 普通文本
    TagOpen,     /// <          标签开始
    TagName,     /// <tag       标签名
    EndTagOpen,  /// </tag      结束标签开始
    EndTagName,  /// </tag      结束标签名称

    BeforeAttributeName,         /// <tag                   标签属性前的空格
    AttributeName,               /// <tag attr              属性名
    AfterAttributeName,          /// <tag attr              属性名后的空格
    BeforeAttributeValue,        /// <tag attr=             属性值前的空格
    AttributeValueDoubleQuoted,  /// <tag attr="value"      双引号属性值
    AttributeValueSingleQuoted,  /// <tag attr='value'      单引号属性值
    AttributeValueUnquoted,      /// <tag attr=value        无引号属性值
    SelfClosingStartTag,         /// <tag attr="value" />   自闭合标签

    DOCTYPE,     /// <!DOCTYPE html>    DOCTYPE
    Comment,     /// <!--comment-->     注释
    ScriptData,  /// 进入 <script> 标签后的原始文本状态
    RAWTEXT,     /// 进入 <style>、<noscript> 等标签后的原始文本状态
    RCDATA,      /// 进入 <textarea>、<title> 标签后的状态（可解析字符实体但不解析标签）
};

struct TokenBuilder {
    std::string                 tag_name;
    std::string                 attr_name;
    std::string                 attr_value;
    bool                        is_void_element = false;
    bool                        is_self_closing = false;
    std::vector<TokenAttribute> attrs;

    void add_attr(const TokenAttribute& attr) {
        attrs.push_back(attr);
    }

    void add_attr(std::string_view name, std::string_view value, bool has_value = true) {
        attrs.emplace_back(name, value, has_value);
    }

    void reset() {
        tag_name.clear();
        attr_name.clear();
        attr_value.clear();
        is_void_element = false;
        is_self_closing = false;
        attrs.clear();
    }

    void finish_current_attribute() {
        if (!attr_name.empty()) {
            add_attr(attr_name, attr_value);
            attr_name.clear();
            attr_value.clear();
        }
    }
};

class Tokenizer : public NonCopyable {
  public:
    explicit Tokenizer(std::string_view source, ErrorHandlingMode mode = ErrorHandlingMode::Strict);
    ~Tokenizer() = default;

    std::optional<Token> next_token();
    std::vector<Token>   tokenize_all();

    [[nodiscard]] bool                           has_more() const noexcept;
    [[nodiscard]] size_t                         position() const noexcept;
    [[nodiscard]] size_t                         total_length() const noexcept;
    [[nodiscard]] const std::vector<ParseError>& get_errors() const noexcept;

    /**
     * @brief 设置错误处理模式
     * @param mode 处理模式
     */
    void set_error_handling_mode(ErrorHandlingMode mode) noexcept;

  private:
    std::optional<Token> consume_data_state();
    std::optional<Token> consume_tag_open_state();
    std::optional<Token> consume_tag_name_state();
    std::optional<Token> consume_end_tag_open_state();
    std::optional<Token> consume_end_tag_name_state();

    // 属性相关状态
    std::optional<Token> consume_before_attribute_name_state();
    std::optional<Token> consume_attribute_name_state();
    std::optional<Token> consume_after_attribute_name_state();
    std::optional<Token> consume_before_attribute_value_state();
    std::optional<Token> consume_attribute_value_double_quoted_state();
    std::optional<Token> consume_attribute_value_single_quoted_state();
    std::optional<Token> consume_attribute_value_unquoted_state();
    std::optional<Token> consume_self_closing_start_tag_state();

    // 注释相关状态
    std::optional<Token> consume_comment_state();

    // DOCTYPE相关状态
    std::optional<Token> consume_doctype_state();

    // 特殊内容状态
    std::optional<Token> consume_script_data_state();
    std::optional<Token> consume_rawtext_state();
    std::optional<Token> consume_rcdata_state();

    [[nodiscard]] char current_char() const noexcept;
    [[nodiscard]] char peek_char(size_t offset = 1) const noexcept;
    void               advance() noexcept;
    void               skip_whitespace() noexcept;

    [[nodiscard]] bool starts_with(std::string_view s) const noexcept;

    Token        create_start_tag_token();
    Token        create_end_tag_token();
    static Token create_text_token(std::string_view data = "");
    static Token create_comment_token(std::string_view comment);
    Token        create_doctype_token();
    Token        create_close_self_token();
    static Token create_done_token();

    void handle_parse_error(ErrorCode code, const std::string& message);
    void record_error(ErrorCode code, const std::string& message);
    void transition_to_data_state();

  private:
    std::string_view  m_source;      // 输入 HTML 字符串
    size_t            m_pos;         // 当前解析位置
    TokenizerState    m_state;       // 当前状态
    ErrorHandlingMode m_error_mode;  // 错误处理模式

    TokenBuilder            m_token_builder;  // Token 构造器
    std::string             m_end_tag;        // 当前结束标签
    std::vector<ParseError> m_errors;         // 错误列表

    std::string m_char_ref_buffer;  // 字符引用缓冲区
};

}  // namespace hps
