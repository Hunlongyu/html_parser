#pragma once

#include <functional>
#include <string>
#include <unordered_set>

namespace hps {

/**
 * @brief 错误处理模式枚举
 *
 * 定义HTML解析过程中遇到错误时的处理策略。
 */
enum class ErrorHandlingMode {
    Strict,   ///< 严格模式，遇到错误立即抛出异常
    Lenient,  ///< 宽松模式，尝试恢复并继续解析
    Ignore    ///< 忽略模式，记录错误但继续解析
};

/**
 * @brief 空白文本处理模式枚举
 *
 * 定义如何处理HTML中的空白字符和文本节点。
 */
enum class WhitespaceMode {
    Preserve,   ///< 保留所有空白文本
    Normalize,  ///< 标准化空白（合并连续空白为单个空格）
    Trim,       ///< 去除首尾空白
    Remove      ///< 完全移除空白文本节点
};

/**
 * @brief 注释处理模式枚举
 *
 * 定义如何处理HTML注释节点。
 */
enum class CommentMode {
    Preserve,    ///< 保留注释
    Remove,      ///< 移除注释
    ProcessOnly  ///< 仅处理但不添加到DOM树
};

/**
 * @brief 解析选项配置类
 *
 * ParserOptions类提供了HTML解析器的各种配置选项，包括错误处理模式、
 * 内容处理选项、性能限制和自定义过滤器等。通过不同的配置组合，
 * 可以满足不同场景下的解析需求。
 */
class ParserOptions {
  public:
    /**
     * @brief 默认构造函数
     *
     * 使用合理的默认值初始化所有选项。
     */
    ParserOptions() = default;

    // 核心解析选项（最重要的基础配置）
    ErrorHandlingMode error_handling = ErrorHandlingMode::Lenient;  ///< 错误处理模式，默认宽松模式

    // 内容处理选项（重要的功能配置）
    CommentMode    comment_mode    = CommentMode::Preserve;     ///< 注释处理模式，默认保留注释
    WhitespaceMode whitespace_mode = WhitespaceMode::Preserve;  ///< 空白文本处理模式，默认保留空白

    // 高级选项（扩展功能配置）
    bool preserve_case = false;  ///< 是否保持标签和属性名大小写，默认转为小写

    // 性能和安全限制（安全相关配置）
    size_t max_tokens                 = 1000000;  ///< 最大Token数量限制
    size_t max_depth                  = 1000;     ///< 最大嵌套深度限制
    size_t max_attributes             = 100;      ///< 单个元素最大属性数限制
    size_t max_attribute_name_length  = 256;      ///< 属性名最大长度限制
    size_t max_attribute_value_length = 8192;     ///< 属性值最大长度限制
    size_t max_text_length            = 1048576;  ///< 文本节点最大长度限制（1MB）

    // 自定义过滤器（高级定制功能）
    std::unordered_set<std::string> void_elements;  ///< 自定义void元素列表

    // 回调函数（最高级的定制功能）
    std::function<bool(const std::string&)>                     tag_filter;        ///< 标签过滤器回调
    std::function<bool(const std::string&, const std::string&)> attribute_filter;  ///< 属性过滤器回调

    // 静态工厂方法（便利构造器，按使用频率排序）
    /**
     * @brief 创建宽松模式配置
     * @return 配置为宽松错误处理的ParserOptions实例
     */
    [[nodiscard]] static ParserOptions lenient() {
        ParserOptions opts;
        opts.error_handling = ErrorHandlingMode::Lenient;
        return opts;
    }

    /**
     * @brief 创建严格模式配置
     * @return 配置为严格错误处理和更严格限制的ParserOptions实例
     */
    [[nodiscard]] static ParserOptions strict() {
        ParserOptions opts;
        opts.error_handling = ErrorHandlingMode::Strict;
        opts.max_tokens     = 100000;
        opts.max_depth      = 100;
        return opts;
    }

    /**
     * @brief 创建性能优化配置
     * @return 配置为高性能解析的ParserOptions实例
     */
    [[nodiscard]] static ParserOptions performance() {
        ParserOptions opts;
        opts.comment_mode    = CommentMode::Remove;
        opts.whitespace_mode = WhitespaceMode::Remove;
        opts.max_tokens      = 10000000;
        return opts;
    }

    /**
     * @brief 创建安全清理配置
     * @return 配置为移除注释的ParserOptions实例
     */
    [[nodiscard]] static ParserOptions sanitized() {
        ParserOptions opts;
        opts.comment_mode = CommentMode::Remove;
        return opts;
    }
};

}  // namespace hps