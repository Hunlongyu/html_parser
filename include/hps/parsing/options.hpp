#pragma once

#include <functional>
#include <string>
#include <unordered_set>

namespace hps {

// 错误处理模式（已存在，保持一致）
enum class ErrorHandlingMode {
    Strict,   // 严格模式，遇到错误立即抛出异常
    Lenient,  // 宽松模式，尝试恢复并继续解析
    Ignore    // 忽略模式，记录错误但继续解析
};

// 空白文本处理模式
enum class WhitespaceMode {
    Preserve,   // 保留所有空白文本
    Normalize,  // 标准化空白（合并连续空白为单个空格）
    Trim,       // 去除首尾空白
    Remove      // 完全移除空白文本节点
};

// 注释处理模式
enum class CommentMode {
    Preserve,    // 保留注释
    Remove,      // 移除注释
    ProcessOnly  // 仅处理但不添加到DOM树
};

// 解析选项配置类
class ParserOptions {
  public:
    // 构造函数 - 提供合理的默认值
    ParserOptions() = default;

    // 基本解析选项
    ErrorHandlingMode error_handling = ErrorHandlingMode::Lenient;  // 宽松的错误处理模式

    // 内容处理选项
    CommentMode    comment_mode    = CommentMode::Preserve;     // 保留注释
    WhitespaceMode whitespace_mode = WhitespaceMode::Preserve;  // 保留空白文本

    // 性能和安全限制
    size_t max_tokens                 = 1000000;  // 最大Token数量
    size_t max_depth                  = 1000;     // 最大嵌套深度
    size_t max_attributes             = 100;      // 单个元素最大属性数
    size_t max_attribute_name_length  = 256;      // 属性名最大长度
    size_t max_attribute_value_length = 8192;     // 属性值最大长度
    size_t max_text_length            = 1048576;  // 文本节点最大长度（1MB）

    // 高级选项
    bool preserve_case = false;  // 是否保持标签和属性名大小写

    // 自定义过滤器
    std::unordered_set<std::string> void_elements;  // 自定义void元素列表

    // 回调函数
    std::function<bool(const std::string&)>                     tag_filter;        // 标签过滤器
    std::function<bool(const std::string&, const std::string&)> attribute_filter;  // 属性过滤器

    // 便利方法
    static ParserOptions strict() {
        ParserOptions opts;
        opts.error_handling = ErrorHandlingMode::Strict;
        opts.max_tokens     = 100000;
        opts.max_depth      = 100;
        return opts;
    }

    static ParserOptions lenient() {
        ParserOptions opts;
        opts.error_handling = ErrorHandlingMode::Lenient;
        return opts;
    }

    static ParserOptions sanitized() {
        ParserOptions opts;
        opts.comment_mode = CommentMode::Remove;
        return opts;
    }

    static ParserOptions performance() {
        ParserOptions opts;
        opts.comment_mode    = CommentMode::Remove;
        opts.whitespace_mode = WhitespaceMode::Remove;
        opts.max_tokens      = 10000000;
        return opts;
    }
};

}  // namespace hps