#pragma once

#include "css_lexer.hpp"
#include "css_selector.hpp"
#include "hps/parsing/options.hpp"
#include "hps/utils/exception.hpp"

#include <memory>
#include <string>
#include <vector>

namespace hps {
/**
 * @brief CSS解析器 - 负责将标记序列解析为CSS选择器AST
 *
 * 该类实现了完整的CSS选择器语法分析功能，支持：
 * - 基本选择器（标签、类、ID、属性、通用）
 * - 组合选择器（后代、子、相邻兄弟、一般兄弟）
 * - 复合选择器（如div.class#id）
 * - 选择器列表（逗号分隔的多个选择器）
 * - CSS3伪类和伪元素
 * - 选择器缓存和性能优化
 */
class CSSParser {
  public:
    /**
     * @brief 构造函数
     * @param selector CSS选择器字符串
     * @param options 解析选项（可选）
     */
    explicit CSSParser(std::string_view selector, const Options& options = Options{});

    /**
     * @brief 解析选择器列表
     * @return 解析后的选择器列表AST
     * @note 处理逗号分隔的多个选择器，如"div, p, .class"
     */
    std::unique_ptr<SelectorList> parse_selector_list();

    /**
     * @brief 解析单个选择器
     * @return 解析后的选择器AST
     * @note 处理单个完整的选择器，如"div.class > p:first-child"
     */
    std::unique_ptr<CSSSelector> parse_selector();

    /**
     * @brief 获取解析过程中收集的错误信息
     * @return 错误信息列表的常量引用
     */
    [[nodiscard]] const std::vector<ParseError>& get_errors() const {
        return m_errors;
    }

    /**
     * @brief 检查是否有解析错误
     * @return 如果有错误返回true，否则返回false
     */
    [[nodiscard]] bool has_errors() const {
        return !m_errors.empty();
    }

    /**
     * @brief 清除所有错误信息
     * @note 用于复用解析器实例时清理之前的错误状态
     */
    void clear_errors() {
        m_errors.clear();
    }

  private:
    CSSLexer                m_lexer;    ///< 词法分析器实例
    Options                 m_options;  ///< 解析选项配置
    std::vector<ParseError> m_errors;   ///< 收集的解析错误列表

    // 核心解析方法

    /**
     * @brief 解析复合选择器
     * @return 复合选择器AST
     * @note 处理如"div.class#id"这样的复合选择器
     */
    std::unique_ptr<CSSSelector> parse_compound_selector();

    /**
     * @brief 解析简单选择器
     * @return 简单选择器AST
     * @note 处理单个基本选择器（标签、类、ID、属性等）
     */
    std::unique_ptr<CSSSelector> parse_simple_selector();

    /**
     * @brief 解析标签选择器
     * @return 标签选择器AST
     * @note 处理如 "div"、"p"、"span" 等标签选择器
     */
    std::unique_ptr<TypeSelector> parse_type_selector();

    /**
     * @brief 解析类选择器
     * @return 类选择器AST
     * @note 处理如 ".class-name" 的类选择器
     */
    std::unique_ptr<ClassSelector> parse_class_selector();

    /**
     * @brief 解析 ID 选择器
     * @return ID 选择器 AST
     * @note 处理如 "#element-id" 的ID选择器
     */
    std::unique_ptr<IdSelector> parse_id_selector();

    /**
     * @brief 解析属性选择器
     * @return 属性选择器 AST
     * @note 处理如 "[attr]"、"[attr=value]"、"[attr*=value]" 等属性选择器
     */
    std::unique_ptr<AttributeSelector> parse_attribute_selector();

    /**
     * @brief 解析伪选择器（伪类或伪元素）
     * @return 伪选择器 AST
     * @note 处理如 ":hover"、"::before" 等伪选择器
     */
    std::unique_ptr<CSSSelector> parse_pseudo_selector();

    /**
     * @brief 解析伪类选择器
     * @return 伪类选择器 AST
     * @note 处理如 ":first-child"、":nth-child(2n+1)" 等伪类
     */
    std::unique_ptr<CSSSelector> parse_pseudo_class();

    /**
     * @brief 解析伪元素选择器
     * @return 伪元素选择器 AST
     * @note 处理如 "::before"、"::after" 等伪元素
     */
    std::unique_ptr<CSSSelector> parse_pseudo_element();

    /**
     * @brief 解析组合器
     * @return 组合器类型
     * @note 识别空格（后代）、>（子）、+（相邻兄弟）、~（一般兄弟）等组合器
     */
    SelectorType parse_combinator();

    /**
     * @brief 解析属性操作符
     * @return 属性操作符类型
     * @note 识别=、*=、^=、$=、~=、|=等属性匹配操作符
     */
    AttributeOperator parse_attribute_operator();

    // 工具方法

    /**
     * @brief 消费指定类型的标记
     * @param expected 期望的标记类型
     * @throws HPSException 如果当前标记类型不匹配
     */
    void consume_token(CSSLexer::CSSTokenType expected);

    /**
     * @brief 检查当前标记是否匹配指定类型
     * @param type 要检查的标记类型
     * @return 如果匹配返回 true，否则返回 false
     */
    bool match_token(CSSLexer::CSSTokenType type);

    /**
     * @brief 跳过空白标记
     * @note 在解析过程中忽略不重要的空白字符
     */
    void skip_whitespace();

    /**
     * @brief 检查是否还有更多标记可解析
     * @return 如果还有标记返回 true，否则返回 false
     */
    bool has_more_tokens();

    /**
     * @brief 添加解析错误
     * @param message 错误消息
     * @note 在非严格模式下收集错误而不是立即抛出异常
     */
    void add_error(const std::string& message);

    /**
     * @brief 添加解析错误（带标记信息）
     * @param message 错误消息
     * @param token 相关的标记
     */
    void add_error(const std::string& message, const CSSLexer::CSSToken& token);

    /**
     * @brief 尝试从解析错误中恢复
     * @return 如果成功恢复返回 true，否则返回 false
     * @note 在遇到语法错误时尝试跳过错误部分继续解析
     */
    bool try_recover_from_error();

    /**
     * @brief 抛出解析错误异常
     * @param message 错误消息
     * @throws HPSException 包含详细错误信息的异常
     */
    [[noreturn]] void throw_parse_error(const std::string& message) const;
};

/**
 * @brief 伪类选择器实现
 *
 * 支持CSS标准中定义的各种伪类选择器，如结构伪类、状态伪类等
 */
class PseudoClassSelector : public CSSSelector {
  public:
    /**
     * @brief 伪类类型枚举
     *
     * 定义了支持的所有伪类类型
     */
    enum class PseudoType {
        FirstChild,    ///< :first-child - 第一个子元素
        LastChild,     ///< :last-child - 最后一个子元素
        NthChild,      ///< :nth-child(n) - 第n个子元素
        NthLastChild,  ///< :nth-last-child(n) - 倒数第n个子元素
        FirstOfType,   ///< :first-of-type - 同类型中的第一个元素
        LastOfType,    ///< :last-of-type - 同类型中的最后一个元素
        OnlyChild,     ///< :only-child - 唯一子元素
        OnlyOfType,    ///< :only-of-type - 同类型中的唯一元素
        Empty,         ///< :empty - 空元素（无子元素和文本内容）
        Root,          ///< :root - 根元素
        Not,           ///< :not(selector) - 否定伪类
        Hover,         ///< :hover - 鼠标悬停状态
        Active,        ///< :active - 激活状态
        Focus,         ///< :focus - 焦点状态
        Visited,       ///< :visited - 已访问链接
        Link,          ///< :link - 未访问链接
        Disabled,      ///< :disabled - 禁用状态
        Enabled,       ///< :enabled - 启用状态
        Checked        ///< :checked - 选中状态
    };

    /**
     * @brief 构造函数
     * @param type 伪类类型
     * @param argument 伪类参数（如nth-child的公式）
     */
    explicit PseudoClassSelector(const PseudoType type, std::string argument = "")
        : CSSSelector(SelectorType::PseudoClass),
          m_pseudo_type(type),
          m_argument(std::move(argument)) {}

    /**
     * @brief 检查元素是否匹配该伪类选择器
     * @param element 要检查的元素
     * @return 如果匹配返回true，否则返回false
     */
    [[nodiscard]] bool matches(const Element& element) const override;

    /**
     * @brief 将选择器转换为字符串表示
     * @return 选择器的字符串形式
     */
    [[nodiscard]] std::string to_string() const override;

    /**
     * @brief 计算伪类选择器的特异性
     * @return 选择器特异性值
     */
    [[nodiscard]] SelectorSpecificity calculate_specificity() const override {
        SelectorSpecificity spec{};
        spec.classes = 1;  // 伪类选择器增加类选择器计数
        return spec;
    }

    /**
     * @brief 获取伪类类型
     * @return 伪类类型
     */
    [[nodiscard]] PseudoType pseudo_type() const noexcept {
        return m_pseudo_type;
    }

    /**
     * @brief 获取伪类参数
     * @return 伪类参数字符串
     */
    [[nodiscard]] const std::string& argument() const noexcept {
        return m_argument;
    }

  private:
    PseudoType  m_pseudo_type;  ///< 伪类类型
    std::string m_argument;     ///< 伪类参数（用于nth-child(n)等带参数的伪类）

    /**
     * @brief 解析nth-child表达式
     * @param expression nth表达式（如"2n+1"、"odd"、"even"）
     * @param index 当前元素的索引（从1开始）
     * @return 是否匹配
     */
    [[nodiscard]] static bool matches_nth_expression(const std::string& expression, int index);

    /**
     * @brief 获取同类型兄弟元素的数量
     * @param element 目标元素
     * @return 同类型兄弟元素数量
     */
    [[nodiscard]] static int count_siblings_of_type(const Element& element);

    /**
     * @brief 获取元素在同类型兄弟中的索引
     * @param element 目标元素
     * @param from_end 是否从末尾开始计数
     * @return 索引（从1开始）
     */
    [[nodiscard]] static int get_type_index(const Element& element, bool from_end = false);
};

/**
 * @brief 伪元素选择器实现
 *
 * 支持CSS标准中定义的伪元素选择器
 */
class PseudoElementSelector : public CSSSelector {
  public:
    /**
     * @brief 伪元素类型枚举
     */
    enum class ElementType {
        Before,      ///< ::before - 元素内容之前插入的虚拟元素
        After,       ///< ::after - 元素内容之后插入的虚拟元素
        FirstLine,   ///< ::first-line - 元素的第一行
        FirstLetter  ///< ::first-letter - 元素的第一个字母
    };

    /**
     * @brief 构造函数
     * @param type 伪元素类型
     */
    explicit PseudoElementSelector(const ElementType type)
        : CSSSelector(SelectorType::PseudoElement),
          m_element_type(type) {}

    /**
     * @brief 检查元素是否匹配该伪元素选择器
     * @param element 要检查的元素
     * @return 如果匹配返回true，否则返回false
     * @note 伪元素通常不直接匹配DOM元素，而是用于样式应用
     */
    [[nodiscard]] bool matches(const Element& element) const override;

    /**
     * @brief 将选择器转换为字符串表示
     * @return 选择器的字符串形式
     */
    [[nodiscard]] std::string to_string() const override;

    /**
     * @brief 获取伪元素类型
     * @return 伪元素类型
     */
    [[nodiscard]] ElementType element_type() const {
        return m_element_type;
    }

    /**
     * @brief 计算伪元素选择器的特异性
     * @return 选择器特异性值
     */
    [[nodiscard]] SelectorSpecificity calculate_specificity() const override {
        SelectorSpecificity spec{};
        spec.elements = 1;  // 伪元素选择器增加元素选择器计数
        return spec;
    }

  private:
    ElementType m_element_type;  ///< 伪元素类型
};
}  // namespace hps