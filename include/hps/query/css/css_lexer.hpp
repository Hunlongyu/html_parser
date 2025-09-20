#pragma once
#include <optional>
#include <string>

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
        Identifier,    ///< 标识符：div, class-name, attr-name
        Hash,          ///< 哈希标记：#id (ID选择器)
        Dot,           ///< 点标记：. (类选择器前缀)
        Star,          ///< 星号：* (通用选择器)
        LeftBracket,   ///< 左方括号：[ (属性选择器开始)
        RightBracket,  ///< 右方括号：] (属性选择器结束)
        Equals,        ///< 等号：= (属性值精确匹配)
        Contains,      ///< 包含操作符：*= (属性值包含匹配)
        StartsWith,    ///< 开始操作符：^= (属性值前缀匹配)
        EndsWith,      ///< 结束操作符：$= (属性值后缀匹配)
        WordMatch,     ///< 单词匹配：~= (属性值单词匹配)
        LangMatch,     ///< 语言匹配：|= (属性值语言匹配)
        String,        ///< 字符串字面量："value" 或 'value'
        Number,        ///< 数字：123, 3.14 (用于nth-child表达式)
        Greater,       ///< 大于号：> (子选择器组合器)
        Plus,          ///< 加号：+ (相邻兄弟选择器组合器)
        Minus,         ///< 减号：- (用于nth-child表达式)
        Tilde,         ///< 波浪号：~ (一般兄弟选择器组合器)
        Comma,         ///< 逗号：, (选择器列表分隔符)
        Whitespace,    ///< 空白字符：空格、制表符、换行符 (后代选择器组合器)
        Colon,         ///< 冒号：: (伪类选择器前缀)
        DoubleColon,   ///< 双冒号：:: (伪元素选择器前缀)
        LeftParen,     ///< 左圆括号：( (函数式伪类参数开始)
        RightParen,    ///< 右圆括号：) (函数式伪类参数结束)
        EndOfFile      ///< 文件结束标记
    };

    /**
     * @brief CSS标记结构体
     *
     * 表示词法分析过程中识别出的单个标记
     */
    struct CSSToken {
        CSSTokenType type;      ///< 标记类型
        std::string  value;     ///< 标记值（指向原始输入字符串的视图）
        size_t       position;  ///< 标记在输入字符串中的起始位置
        size_t       length;    ///< 标记长度

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
     */
    explicit CSSLexer(std::string_view input);

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
    std::string             m_processed_input;  ///< 预处理后的输入字符串（移除注释等）
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
    std::string read_identifier();

    /**
     * @brief 读取字符串字面量
     * @param quote 引号字符（'或"）
     * @return 字符串内容的视图（不包含引号）
     * @note 支持转义字符处理
     */
    std::string read_string(char quote);

    /**
     * @brief 读取数字
     * @return 数字的字符串视图
     * @note 用于解析nth-child等函数式伪类的参数
     */
    std::string read_number();

    /**
     * @brief 更新位置信息
     * @param c 当前字符
     * @note 根据字符类型更新行号和列号
     */
    void update_position(char c);

    /**
     * @brief 抛出词法分析错误
     * @param message 错误消息
     * @throws HPSException 包含详细错误信息的异常
     */
    [[noreturn]] void throw_lexer_error(const std::string& message) const;
};
}  // namespace hps