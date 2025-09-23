#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace hps {

/**
 * @brief XPath 词法标记类型枚举
 *
 * 定义了 XPath 1.0 标准中所有支持的词法标记类型，
 * 按照功能分组组织，便于理解和维护。
 */
enum class XPathTokenType {
    // ========== 基本符号 ==========
    SLASH,         ///< / - 路径分隔符
    DOUBLE_SLASH,  ///< // - 递归下降
    DOT,           ///< . - 当前节点
    DOUBLE_DOT,    ///< .. - 父节点
    AT,            ///< @ - 属性标识符
    STAR,          ///< * - 通配符
    MULTIPLY,      ///< * - 乘法操作符
    COLON,         ///< : - 命名空间分隔符
    DOUBLE_COLON,  ///< :: - 轴分隔符
    DOLLAR,        ///< $ - 变量引用标识符

    // ========== 括号和分隔符 ==========
    LPAREN,    ///< ( - 左圆括号
    RPAREN,    ///< ) - 右圆括号
    LBRACKET,  ///< [ - 左方括号（谓词开始）
    RBRACKET,  ///< ] - 右方括号（谓词结束）
    COMMA,     ///< , - 参数分隔符
    UNION,     ///< | - 联合操作符

    // ========== 比较操作符 ==========
    EQUAL,          ///< = - 等于
    NOT_EQUAL,      ///< != - 不等于
    LESS,           ///< < - 小于
    LESS_EQUAL,     ///< <= - 小于等于
    GREATER,        ///< > - 大于
    GREATER_EQUAL,  ///< >= - 大于等于

    // ========== 算术操作符 ==========
    PLUS,   ///< + - 加法
    MINUS,  ///< - - 减法

    // ========== 逻辑操作符 ==========
    AND,  ///< and - 逻辑与
    OR,   ///< or - 逻辑或

    // ========== 数学操作符 ==========
    DIVIDE,  ///< div - 除法
    MODULO,  ///< mod - 取模

    // ========== 轴标识符 ==========
    AXIS_ANCESTOR,            ///< ancestor:: - 祖先轴
    AXIS_ANCESTOR_OR_SELF,    ///< ancestor-or-self:: - 祖先或自身轴
    AXIS_ATTRIBUTE,           ///< attribute:: - 属性轴
    AXIS_CHILD,               ///< child:: - 子元素轴
    AXIS_DESCENDANT,          ///< descendant:: - 后代轴
    AXIS_DESCENDANT_OR_SELF,  ///< descendant-or-self:: - 后代或自身轴
    AXIS_FOLLOWING,           ///< following:: - 后续轴
    AXIS_FOLLOWING_SIBLING,   ///< following-sibling:: - 后续兄弟轴
    AXIS_NAMESPACE,           ///< namespace:: - 命名空间轴
    AXIS_PARENT,              ///< parent:: - 父元素轴
    AXIS_PRECEDING,           ///< preceding:: - 前驱轴
    AXIS_PRECEDING_SIBLING,   ///< preceding-sibling:: - 前驱兄弟轴
    AXIS_SELF,                ///< self:: - 自身轴

    // ========== 节点类型测试 ==========
    NODE_TYPE_COMMENT,                 ///< comment() - 注释节点
    NODE_TYPE_TEXT,                    ///< text() - 文本节点
    NODE_TYPE_PROCESSING_INSTRUCTION,  ///< processing-instruction() - 处理指令节点
    NODE_TYPE_NODE,                    ///< node() - 任意节点

    // ========== 标识符和字面量 ==========
    FUNCTION_NAME,   ///< XPath 内置函数名
    IDENTIFIER,      ///< 普通标识符（元素名、属性名等）
    VARIABLE,        ///< 变量引用（$name）
    STRING_LITERAL,  ///< 字符串字面量
    NUMBER_LITERAL,  ///< 数字字面量

    // ========== 特殊标记 ==========
    END_OF_FILE,  ///< 输入结束标记
    ERR,          ///< 错误标记
};

/**
 * @brief XPath 词法标记结构体
 *
 * 表示词法分析过程中识别出的一个标记，包含类型、值和位置信息。
 */
struct XPathToken {
    XPathTokenType type;      ///< 标记类型
    std::string    value;     ///< 标记值（原始文本）
    size_t         position;  ///< 标记在输入中的位置

    // 添加默认构造函数
    XPathToken()
        : type(XPathTokenType::END_OF_FILE),
          value(""),
          position(0) {}

    /**
     * @brief 构造函数
     * @param t 标记类型
     * @param v 标记值
     * @param pos 标记位置
     */
    XPathToken(const XPathTokenType t, const std::string_view v, const size_t pos)
        : type(t),
          value(v),
          position(pos) {}
};

/**
 * @brief XPath 词法分析器
 *
 * 负责将 XPath 表达式字符串分解为词法标记序列，
 * 支持 XPath 1.0 标准的完整语法。
 *
 * 主要功能：
 * - 识别操作符、关键字、标识符
 * - 解析字符串和数字字面量
 * - 区分函数名和普通标识符
 * - 支持轴标识符和节点类型测试
 * - 提供前瞻功能
 */
class XPathLexer {
  public:
    /**
     * @brief 构造函数
     * @param input XPath 表达式字符串
     */
    explicit XPathLexer(std::string_view input);

    // ========== 主要接口 ==========

    /**
     * @brief 获取下一个标记
     * @return 下一个词法标记
     */
    XPathToken next_token();

    /**
     * @brief 预览下一个标记（不消费）
     * @return 下一个词法标记的副本
     */
    XPathToken peek_token() const;

    /**
     * @brief 消费当前预览的标记
     */
    void consume_token();

    // ========== 状态查询 ==========

    /**
     * @brief 检查是否还有更多标记
     * @return 如果还有标记返回 true，否则返回 false
     */
    bool has_next() const;

    /**
     * @brief 获取当前位置
     * @return 当前在输入字符串中的位置
     */
    size_t position() const;

  private:
    // ========== 成员变量 ==========
    std::string                         m_input;         ///< 输入的 XPath 表达式
    size_t                              m_position;      ///< 当前解析位置
    mutable std::unique_ptr<XPathToken> m_peeked_token;  ///< 预览的标记缓存

    // ========== 字符操作 ==========

    /**
     * @brief 获取当前字符
     * @return 当前位置的字符
     */
    char current_char() const;

    /**
     * @brief 预览下一个字符
     * @return 下一个位置的字符
     */
    char peek_char() const;

    /**
     * @brief 前进一个字符位置
     */
    void advance();

    /**
     * @brief 跳过空白字符
     */
    void skip_whitespace();

    // ========== 扫描方法 ==========

    /**
     * @brief 扫描字符串字面量
     * @return 字符串字面量标记
     */
    XPathToken scan_string_literal();

    /**
     * @brief 扫描数字字面量
     * @return 数字字面量标记
     */
    XPathToken scan_number_literal();

    /**
     * @brief 扫描标识符或关键字
     * @return 标识符、关键字、轴标识符或函数名标记
     */
    XPathToken scan_identifier_or_keyword();

    /**
     * @brief 扫描操作符
     * @return 操作符标记，如果不是操作符则返回错误标记
     */
    XPathToken scan_operator();

    // ========== 辅助函数 ==========

    /**
     * @brief 检查给定名称是否为 XPath 内置函数
     * @param name 要检查的名称
     * @return 如果是内置函数返回 true，否则返回 false
     */
    static bool is_xpath_function(const std::string& name);
};

}  // namespace hps