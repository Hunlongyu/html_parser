#pragma once
#include "parsing/token.hpp"
#include "utils/noncopyable.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace hps {

enum class TokenizerState {
    Data,        /// 普通文本
    TagOpen,     /// <   标签开始
    TagName,     /// <tag   标签名
    EndTagOpen,  /// </tag   结束标签开始
    EndTagName,  /// </tag   结束标签名称

    BeforeAttributeName,         /// <tag   标签属性前的空格
    AttributeName,               /// <tag attr   属性名
    AfterAttributeName,          /// <tag attr   属性名后的空格
    BeforeAttributeValue,        /// <tag attr=  属性值前的空格
    AttributeValueDoubleQuoted,  /// <tag attr="value"   双引号属性值
    AttributeValueSingleQuoted,  /// <tag attr='value'   单引号属性值
    AttributeValueUnquoted,      /// <tag attr=value   无引号属性值
    SelfClosingStartTag,         /// <tag attr="value" />   自闭合标签

    CommentStart,    /// <!--   注释开始
    Comment,         /// <!--comment-->   注释
    CommentEndDash,  /// <!--comment--->   注释结束减号
    CommentEnd,      /// <!--comment-->   注释结束

    DOCTYPE,           /// <!DOCTYPE html>   DOCTYPE
    DOCTYPEName,       /// <!DOCTYPE html>   DOCTYPE名称
    AfterDOCTYPEName,  /// <!DOCTYPE html>   DOCTYPE名称后

    ScriptData,  /// 进入 <script> 标签后的原始文本状态
    RAWTEXT,     /// 进入 <style>、<noscript> 等标签后的原始文本状态
    RCDATA,      // 进入 <textarea>、<title> 标签后的状态（可解析字符实体但不解析标签）

    CharacterReference,         // 遇到 & 字符，进入字符引用解析
    NamedCharacterReference,    // 解析命名字符引用（如 &amp;）
    NumericCharacterReference,  // 解析数字字符引用（如 &#64;）

    MarkupDeclarationOpen,  // 遇到 <! 标记声明开始（用于处理 <!DOCTYPE 和 <!--）
    CDATASection,           // 处理 <![CDATA[ ... ]]> 节
};

struct TokenBuilder {
    std::string                 tag_name;
    std::string                 attr_name;
    std::string                 attr_value;
    bool                        is_void_element = false;
    bool                        is_self_closing = false;
    std::vector<TokenAttribute> attrs;

    void reset() {
        tag_name.clear();
        attr_name.clear();
        attr_value.clear();
        is_void_element = false;
        is_self_closing = false;
        attrs.clear();
    }
};

class Tokenizer : public NonCopyable {
  public:
    explicit Tokenizer(std::string_view source);
    ~Tokenizer() = default;

    std::optional<Token> next_token();
    std::vector<Token>   tokenize_all();

    bool   has_more() const noexcept;
    size_t position() const noexcept;
    size_t total_length() const noexcept;

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
    std::optional<Token> consume_comment_start_state();
    std::optional<Token> consume_comment_state();
    std::optional<Token> consume_comment_end_dash_state();
    std::optional<Token> consume_comment_end_state();

    // DOCTYPE相关状态
    std::optional<Token> consume_doctype_state();
    std::optional<Token> consume_doctype_name_state();
    std::optional<Token> consume_after_doctype_name_state();

    // 特殊内容状态
    std::optional<Token> consume_script_data_state();
    std::optional<Token> consume_rawtext_state();
    std::optional<Token> consume_rcdata_state();

    // 字符引用相关状态
    std::optional<Token> consume_character_reference_state();
    std::optional<Token> consume_named_character_reference_state();
    std::optional<Token> consume_numeric_character_reference_state();

    // 标记声明和CDATA
    std::optional<Token> consume_markup_declaration_open_state();
    std::optional<Token> consume_cdata_section_state();

    char current_char() const noexcept;
    char peek_char(size_t offset = 1) const noexcept;
    void advance() noexcept;
    void skip_whitespace() noexcept;

    bool        starts_with(std::string_view s) const noexcept;
    static bool is_whitespace(char c) noexcept;
    static bool is_alpha(char c) noexcept;
    static bool is_alnum(char c) noexcept;
    static char to_lower(char c) noexcept;
    static bool is_void_element_name(std::string_view n) noexcept;

    Token create_start_tag_token();
    Token create_end_tag_token();
    Token create_text_token();
    Token create_comment_token();
    Token create_doctype_token();
    Token create_close_self_token();
    Token create_done_token();

  private:
    std::string_view m_source;  // 输入 HTML 字符串
    size_t           m_pos;     // 当前解析位置
    TokenizerState   m_state;   // 当前状态

    TokenBuilder m_token_builder;  // Token 构造器
    std::string  m_end_tag;        // 当前结束标签
};

}  // namespace hps
