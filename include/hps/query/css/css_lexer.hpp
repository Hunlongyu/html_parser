#pragma once
#include "hps/utils/string_pool.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hps {

/**
 * @brief CSS词法分析器 - 负责将CSS选择器字符串分解为标记序列
 *
 * 该类实现了完整的CSS选择器词法分析功能，支持：
 * - 基本选择器标记（标签、类、ID、属性等）
 * - 组合器标记（空格、>、+、~等）
 * - CSS3伪类和伪元素标记
 * - 字符串和标识符解析
 * - 错误位置跟踪和报告
 */
class CSSLexer {
  public:
    /**
     * @brief CSS标记类型枚举
     *
     * 定义了CSS选择器中所有可能的标记类型
     */
    enum class CSSTokenType : std::uint8_t {
        // --- 基础值类型 (Values) ---
        Identifier,  ///< 标识符：div, class-name
        String,      ///< 字符串："foo", 'bar'
        Number,      ///< 数字：123, 3.14

        // --- 简单选择器前缀 (Simple Selectors) ---
        Hash,         ///< ID选择器：#id
        Dot,          ///< 类选择器：.class
        Star,         ///< 通用选择器：*
        Colon,        ///< 伪类前缀：:hover
        DoubleColon,  ///< 伪元素前缀：::before
        Pipe,         ///< 命名空间分隔符：| (ns|tag)

        // --- 组合器 (Combinators) ---
        Whitespace,  ///< 后代组合器：(空格)
        Greater,     ///< 子元素组合器：>
        Plus,        ///< 相邻兄弟组合器：+
        Tilde,       ///< 一般兄弟组合器：~
        Column,      ///< 列组合器：||

        // --- 属性选择器操作符 (Attribute Matchers) ---
        LeftBracket,   ///< 属性开始：[
        RightBracket,  ///< 属性结束：]
        Equals,        ///< 精确匹配：=
        Contains,      ///< 包含匹配：*=
        StartsWith,    ///< 前缀匹配：^=
        EndsWith,      ///< 后缀匹配：$=
        WordMatch,     ///< 单词匹配：~=
        LangMatch,     ///< 语言匹配：|=

        // --- 函数与表达式 (Functions & Expressions) ---
        LeftParen,   ///< 函数开始：(
        RightParen,  ///< 函数结束：)
        Comma,       ///< 列表分隔符：,
        Minus,       ///< 减号：- (nth-child)

        // --- 控制标记 (Control) ---
        EndOfFile,  ///< 文件结束
        Error       ///< 错误
    };

    /**
     * @brief CSS标记结构体
     *
     * 表示词法分析过程中识别出的单个标记
     */
    struct CSSToken {
        CSSTokenType     type;      ///< 标记类型
        std::string_view value;     ///< 标记值（指向 StringPool 中的视图）
        size_t           position;  ///< 标记在输入字符串中的起始位置
        size_t           length;    ///< 标记长度

        /**
         * @brief 构造函数
         * @param t 标记类型
         * @param v 标记值
         * @param pos 起始位置
         * @param len 长度（默认为值的长度）
         */
        CSSToken(const CSSTokenType t, const std::string_view v, const size_t pos, const size_t len = 0)
            : type(t),
              value(v),
              position(pos),
              length(len > 0 ? len : v.length()) {}
    };

    /**
     * @brief 构造函数
     * @param input CSS选择器字符串
     * @param pool 字符串池，用于存储处理后的字符串
     */
    explicit CSSLexer(std::string_view input, StringPool& pool);

    /**
     * @brief 获取下一个标记
     * @return 下一个标记
     * @note 该方法会消费标记，即调用后标记不再可用
     */
    CSSToken next_token();

    /**
     * @brief 预览下一个标记
     * @return 下一个标记
     * @note 该方法不会消费标记，可以多次调用获取同一标记
     */
    CSSToken peek_token();

    /**
     * @brief 检查是否还有更多标记
     * @return 如果还有标记返回true，否则返回false
     */
    [[nodiscard]] bool has_more_tokens() const;

    /**
     * @brief 获取当前字符位置
     * @return 当前字符在输入字符串中的位置
     */
    [[nodiscard]] size_t current_position() const {
        return m_position;
    }

    /**
     * @brief 获取当前行号
     * @return 当前行号（从1开始）
     */
    [[nodiscard]] size_t current_line() const {
        return m_line;
    }

    /**
     * @brief 获取当前列号
     * @return 当前列号（从1开始）
     */
    [[nodiscard]] size_t current_column() const {
        return m_column;
    }

    /**
     * @brief 重置词法分析器状态
     * @param input 新的输入字符串
     * @note 用于复用词法分析器实例解析不同的选择器
     */
    void reset(std::string_view input);

  private:
    std::string_view        m_input;            ///< 原始输入字符串
    std::string_view        m_processed_input;  ///< 预处理后的输入字符串（存储在 StringPool 中）
    StringPool&             m_pool;             ///< 字符串池引用
    size_t                  m_position;         ///< 当前字符位置
    size_t                  m_line;             ///< 当前行号
    size_t                  m_column;           ///< 当前列号
    std::optional<CSSToken> m_current_token;    ///< 缓存的当前标记（用于peek操作）

    /**
     * @brief 预处理输入字符串
     *
     * 执行以下预处理操作：
     * - 移除CSS注释
     * - 规范化空白字符
     * - 处理转义字符
     */
    void preprocess();

    /**
     * @brief 读取下一个标记
     * @return 读取到的标记
     * @note 这是实际执行词法分析的核心方法
     */
    CSSToken read_next_token();

    /**
     * @brief 获取当前字符
     * @return 当前位置的字符，如果到达末尾返回'\0'
     */
    [[nodiscard]] char current_char() const;

    /**
     * @brief 预览指定偏移位置的字符
     * @param offset 偏移量（默认为1，即下一个字符）
     * @return 指定位置的字符
     */
    [[nodiscard]] char peek_char(size_t offset = 1) const;

    /**
     * @brief 前进到下一个字符
     * @note 同时更新行号和列号信息
     */
    void advance();

    /**
     * @brief 跳过空白字符
     * @note 跳过空格、制表符、换行符等
     */
    bool skip_whitespace();

    /**
     * @brief 读取标识符
     * @return 标识符的字符串视图
     * @note 标识符可以包含字母、数字、下划线和连字符
     */
    std::string_view read_identifier();

    /**
     * @brief 读取字符串字面量
     * @param quote 引号字符（'或"）
     * @return 字符串内容的视图（不包含引号）
     * @note 支持转义字符处理
     */
    std::string_view read_string(char quote);

    /**
     * @brief 读取数字
     * @return 数字的字符串视图
     * @note 用于解析nth-child等函数式伪类的参数
     */
    std::string_view read_number();

    /**
     * @brief 更新位置信息
     * @param c 当前字符
     * @note 根据字符类型更新行号和列号
     */
    void update_position(char c);

    /**
     * @brief 创建错误标记
     * @param message 错误消息
     * @return 错误标记
     */
    CSSToken create_error_token(const std::string& message);
};
}  // namespace hps