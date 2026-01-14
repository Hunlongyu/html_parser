#pragma once
#include "hps/hps_fwd.hpp"
#include "hps/parsing/options.hpp"
#include "hps/parsing/token.hpp"
#include "hps/parsing/token_builder.hpp"
#include "hps/utils/exception.hpp"

#include <optional>

namespace hps {

/**
 * @brief HTML词法分析器
 *
 * Tokenizer类负责将HTML源代码字符串解析为Token序列。它实现了基于状态机的词法分析算法，
 * 能够正确处理HTML标签、属性、文本内容、注释、DOCTYPE声明等各种HTML元素。
 *
 * 主要功能：
 * - 逐个解析HTML Token（标签、文本、注释等）
 * - 支持批量解析整个HTML文档
 * - 提供详细的错误处理和报告机制
 * - 支持不同的错误处理模式（严格模式、宽松模式等）
 *
 * 使用示例：
 * @code
 * std::string html = "<div class='test'>Hello</div>";
 * Tokenizer tokenizer(html);
 * auto tokens = tokenizer.tokenize_all();
 * @endcode
 *
 * @note 该类继承自NonCopyable，不支持拷贝操作，但支持移动语义
 */
class Tokenizer : public NonCopyable {
  public:
    // ==================== 构造函数和析构函数 ====================

    /**
     * @brief 构造HTML词法分析器
     * @param source HTML 源代码字符串视图
     * @param options 解析选项
     */
    explicit Tokenizer(std::string_view source, const Options& options);

    /**
     * @brief 析构函数
     */
    ~Tokenizer() = default;

    // ==================== 核心解析方法 ====================

    /**
     * @brief 解析下一个 Token
     *
     * 从当前位置开始解析下一个 HTML Token。如果解析成功，返回包含 Token 的 optional；
     * 如果到达输入末尾，返回 DONE类型的 Token；如果解析过程中遇到错误，根据错误处理模式
     * 决定是抛出异常还是返回错误 Token。
     *
     * @return 解析得到的 Token，如果到达输入末尾则返回 nullopt
     */
    [[nodiscard]] std::optional<Token> next_token();

    /**
     * @brief 解析所有Token
     *
     * 一次性解析整个 HTML 源代码，返回包含所有 Token 的向量。这是一个便利方法，
     * 内部会重复调用 next_token()直到解析完成。
     *
     * @return 包含所有解析Token的向量
     */
    [[nodiscard]] std::vector<Token> tokenize_all();

    // ==================== 状态查询方法 ====================

    /**
     * @brief 检查是否还有更多字符需要解析
     * @return 如果还有未解析的字符则返回 true，否则返回 false
     */
    [[nodiscard]] bool has_more() const noexcept;

    /**
     * @brief 获取当前解析位置
     * @return 当前字符在源字符串中的索引位置
     */
    [[nodiscard]] size_t position() const noexcept;

    /**
     * @brief 获取源字符串的总长度
     * @return 源HTML字符串的字符总数
     */
    [[nodiscard]] size_t total_length() const noexcept;

    /**
     * @brief 获取解析过程中收集的所有错误
     * @return 包含所有ParseError对象的向量引用
     */
    [[nodiscard]] const std::vector<HPSError>& get_errors() const noexcept;

  private:
    // ==================== 状态处理方法 ====================

    /**
     * @brief 处理 Data 状态（普通文本内容）
     * @return 解析得到的文本 Token，如果没有文本内容则返回 nullopt
     */
    std::optional<Token> consume_data_state();

    /**
     * @brief 处理 TagOpen 状态（标签开始'<'）
     * @return 根据后续字符决定的 Token，通常为 nullopt（状态转换）
     */
    std::optional<Token> consume_tag_open_state();

    /**
     * @brief 处理 TagName 状态（标签名称解析）
     * @return 完整的开始标签 Token 或 nullopt（继续解析属性）
     */
    std::optional<Token> consume_tag_name_state();

    /**
     * @brief 处理 EndTagOpen 状态（结束标签开始'</'）
     * @return 通常返回 nullopt，进行状态转换
     */
    std::optional<Token> consume_end_tag_open_state();

    /**
     * @brief 处理 EndTagName 状态（结束标签名称）
     * @return 完整的结束标签 Token
     */
    std::optional<Token> consume_end_tag_name_state();

    // ==================== 属性解析状态方法 ====================

    /**
     * @brief 处理 BeforeAttributeName 状态（属性名前的空白）
     * @return 通常返回 nullopt，进行状态转换或返回完整标签
     */
    std::optional<Token> consume_before_attribute_name_state();

    /**
     * @brief 处理 AttributeName 状态（属性名解析）
     * @return 通常返回 nullopt，继续解析属性值
     */
    std::optional<Token> consume_attribute_name_state();

    /**
     * @brief 处理 AfterAttributeName 状态（属性名后的空白或'='）
     * @return 通常返回 nullopt，进行状态转换
     */
    std::optional<Token> consume_after_attribute_name_state();

    /**
     * @brief 处理 BeforeAttributeValue 状态（属性值前的空白或引号）
     * @return 通常返回 nullopt，进行状态转换
     */
    std::optional<Token> consume_before_attribute_value_state();

    /**
     * @brief 处理 AttributeValueDoubleQuoted 状态（双引号属性值）
     * @return 通常返回 nullopt，继续收集属性值
     */
    std::optional<Token> consume_attribute_value_double_quoted_state();

    /**
     * @brief 处理 AttributeValueSingleQuoted 状态（单引号属性值）
     * @return 通常返回 nullopt，继续收集属性值
     */
    std::optional<Token> consume_attribute_value_single_quoted_state();

    /**
     * @brief 处理 AttributeValueUnquoted 状态（无引号属性值）
     * @return 可能返回完整标签 Token 或 nullopt
     */
    std::optional<Token> consume_attribute_value_unquoted_state();

    /**
     * @brief 处理 SelfClosingStartTag 状态（自闭合标签'/>'）
     * @return 自闭合标签 Token
     */
    std::optional<Token> consume_self_closing_start_tag_state();

    // ==================== 特殊内容状态方法 ====================

    /**
     * @brief 处理 Comment 状态（HTML 注释）
     * @return 注释 Token
     */
    std::optional<Token> consume_comment_state();

    /**
     * @brief 处理 DOCTYPE 状态（文档类型声明）
     * @return DOCTYPE Token
     */
    std::optional<Token> consume_doctype_state();

    /**
     * @brief 处理 ScriptData 状态（script 标签内的原始文本）
     * @return 脚本内容的文本 Token
     */
    std::optional<Token> consume_script_data_state();

    /**
     * @brief 处理 RAWTEXT 状态（style、noscript 等标签的原始文本）
     * @return 原始文本 Token
     */
    std::optional<Token> consume_rawtext_state();

    /**
     * @brief 处理 RCDATA 状态（textarea、title 等标签的文本，可解析字符实体）
     * @return 处理后的文本 Token
     */
    std::optional<Token> consume_rcdata_state();

    // ==================== 字符操作辅助方法 ====================

    /**
     * @brief 获取当前位置的字符
     * @return 当前字符，如果超出范围则返回'\0'
     */
    [[nodiscard]] char current_char() const noexcept;

    /**
     * @brief 向前查看指定偏移量的字符
     * @param offset 向前查看的字符数，默认为1
     * @return 指定位置的字符，如果超出范围则返回'\0'
     */
    [[nodiscard]] char peek_char(size_t offset = 1) const noexcept;

    /**
     * @brief 将当前位置向前移动一个字符
     */
    void advance() noexcept;

    /**
     * @brief 跳过当前位置开始的所有空白字符
     */
    void skip_whitespace() noexcept;

    // ==================== 字符串匹配辅助方法 ====================

    /**
     * @brief 检查当前位置是否以指定字符串开头
     * @param s 要匹配的字符串
     * @return 如果当前位置以指定字符串开头则返回true
     */
    [[nodiscard]] bool starts_with(std::string_view s) const noexcept;

    // ==================== Token创建方法 ====================

    /**
     * @brief 创建开始标签 Token
     * @return 包含当前标签信息的开始标签 Token
     */
    Token create_start_tag_token();

    /**
     * @brief 创建结束标签 Token
     * @return 包含当前结束标签信息的 Token
     */
    Token create_end_tag_token();

    /**
     * @brief 创建文本 Token
     * @param data 文本内容，默认为空字符串
     * @return 包含指定文本的 Token
     */
    static Token create_text_token(std::string_view data = "");

    /**
     * @brief 创建注释 Token
     * @param comment 注释内容
     * @return 包含注释内容的 Token
     */
    static Token create_comment_token(std::string_view comment);

    /**
     * @brief 创建 DOCTYPE Token
     * @return DOCTYPE类型的 Token
     */
    Token create_doctype_token();

    /**
     * @brief 创建自闭合标签 Token
     * @return 自闭合标签 Token
     */
    Token create_close_self_token();

    /**
     * @brief 创建解析完成 Token
     * @return 表示解析结束的 DONE 类型 Token
     */
    static Token create_done_token();

    // ==================== 错误处理方法 ====================

    /**
     * @brief 处理解析错误
     * @param code 错误代码
     * @param message 错误消息
     */
    void handle_parse_error(ErrorCode code, const std::string& message);

    /**
     * @brief 记录解析错误到错误列表
     * @param code 错误代码
     * @param message 错误消息
     */
    void record_error(ErrorCode code, const std::string& message);

    /**
     * @brief 转换到Data状态
     */
    void transition_to_data_state();

  private:
    // ==================== 核心状态成员变量 ====================

    std::string_view m_source;   ///< 输入HTML字符串视图，保存待解析的源代码
    size_t           m_pos;      ///< 当前解析位置索引，指向下一个要处理的字符
    TokenizerState   m_state;    ///< 当前词法分析器状态，控制解析行为
    const Options&   m_options;  ///< 解析配置

    // ==================== 解析辅助成员变量 ====================

    TokenBuilder          m_token_builder;    ///< Token构造器，用于逐步构建复杂的Token对象
    std::string           m_end_tag;          ///< 当前正在解析的结束标签名称缓存
    std::vector<HPSError> m_errors;           ///< 解析过程中收集的所有错误信息列表
    std::string           m_char_ref_buffer;  ///< 字符引用解析缓冲区，用于处理HTML实体
};

}  // namespace hps
