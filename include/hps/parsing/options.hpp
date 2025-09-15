#pragma once

#include <functional>
#include <mutex>
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
 * @brief 全局解析选项配置单例类
 *
 * Options类提供了HTML解析器的各种配置选项，包括错误处理模式、
 * 内容处理选项、性能限制和自定义过滤器等。作为单例类，确保
 * 全局配置的一致性和可访问性。
 */
class Options {
  private:
    // 私有构造函数，防止外部实例化
    Options() = default;

    // 互斥锁，保证线程安全
    mutable std::mutex mutex_;

    // 默认void元素列表
    static const std::unordered_set<std::string_view>& get_default_void_elements() {
        static const std::unordered_set<std::string_view> default_void_elements = {
            // HTML5 标准 void 元素（不能有结束标签）
            "area",    // 图像映射区域
            "base",    // 文档基础URL
            "br",      // 换行符
            "col",     // 表格列
            "embed",   // 嵌入内容
            "hr",      // 水平分割线
            "img",     // 图像
            "input",   // 输入控件
            "link",    // 外部资源链接
            "meta",    // 元数据
            "param",   // 对象参数
            "source",  // 媒体资源
            "track",   // 文本轨道
            "wbr",     // 可选换行点

            // HTML4 遗留 void 元素（向后兼容）
            "basefont",  // 基础字体（已废弃但仍需支持）
            "frame",     // 框架（已废弃但仍需支持）
            "isindex",   // 索引输入（已废弃但仍需支持）

            // 常见的自闭合元素（虽然技术上不是void，但通常写成自闭合形式）
            "command",   // 命令按钮（HTML5.1中移除，但仍可能遇到）
            "keygen",    // 密钥生成器（已废弃但可能遇到）
            "menuitem",  // 菜单项（实验性，可能遇到）

            // XML风格的自闭合元素（在XHTML中常见）
            "!doctype",  // DOCTYPE声明（特殊处理）
            "?xml",      // XML声明（特殊处理）

            // MathML中的常见void元素
            "mspace",  // MathML空格
            "none",    // MathML空元素

            // 自定义元素和Web Components（常见的自闭合模式）
            "slot",  // Web Components插槽（可以自闭合）
        };
        return default_void_elements;
    }

  public:
    // 删除拷贝构造和赋值操作
    Options(const Options&)            = delete;
    Options& operator=(const Options&) = delete;
    Options(Options&&)                 = delete;
    Options& operator=(Options&&)      = delete;

    /**
     * @brief 获取单例实例
     * @return Options单例引用
     */
    static Options& instance() {
        static Options instance;
        return instance;
    }

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

    /**
     * @brief 判断是否为void元素（线程安全）
     * @param tag_name 标签名
     * @return 如果是void元素返回true，否则返回false
     */
    bool is_void_element(const std::string_view tag_name) const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);

        // 优先使用用户自定义的void_elements
        if (!void_elements.empty()) {
            return void_elements.contains(std::string(tag_name));
        }

        // 使用默认的void元素列表
        return get_default_void_elements().contains(tag_name);
    }

    /**
     * @brief 设置自定义void元素列表（线程安全）
     * @param elements 自定义void元素集合
     */
    void set_void_elements(const std::unordered_set<std::string>& elements) {
        std::lock_guard<std::mutex> lock(mutex_);
        void_elements = elements;
    }

    /**
     * @brief 添加自定义void元素（线程安全）
     * @param element 要添加的void元素名
     */
    void add_void_element(const std::string& element) {
        std::lock_guard<std::mutex> lock(mutex_);
        void_elements.insert(element);
    }

    /**
     * @brief 移除自定义void元素（线程安全）
     * @param element 要移除的void元素名
     */
    void remove_void_element(const std::string& element) {
        std::lock_guard<std::mutex> lock(mutex_);
        void_elements.erase(element);
    }

    /**
     * @brief 清空自定义void元素列表，恢复使用默认列表（线程安全）
     */
    void clear_void_elements() {
        std::lock_guard<std::mutex> lock(mutex_);
        void_elements.clear();
    }

    /**
     * @brief 重置所有配置为默认值（线程安全）
     */
    void reset_to_defaults() {
        std::lock_guard<std::mutex> lock(mutex_);
        error_handling             = ErrorHandlingMode::Lenient;
        comment_mode               = CommentMode::Preserve;
        whitespace_mode            = WhitespaceMode::Preserve;
        preserve_case              = false;
        max_tokens                 = 1000000;
        max_depth                  = 1000;
        max_attributes             = 100;
        max_attribute_name_length  = 256;
        max_attribute_value_length = 8192;
        max_text_length            = 1048576;
        void_elements.clear();
        tag_filter       = nullptr;
        attribute_filter = nullptr;
    }

    // 静态工厂方法（便利配置器，按使用频率排序）
    /**
     * @brief 配置为宽松模式
     */
    void configure_lenient() {
        std::lock_guard<std::mutex> lock(mutex_);
        error_handling = ErrorHandlingMode::Lenient;
    }

    /**
     * @brief 配置为严格模式
     */
    void configure_strict() {
        std::lock_guard<std::mutex> lock(mutex_);
        error_handling = ErrorHandlingMode::Strict;
        max_tokens     = 100000;
        max_depth      = 100;
    }

    /**
     * @brief 配置为性能优化模式
     */
    void configure_performance() {
        std::lock_guard<std::mutex> lock(mutex_);
        comment_mode    = CommentMode::Remove;
        whitespace_mode = WhitespaceMode::Remove;
        max_tokens      = 10000000;
    }

    /**
     * @brief 配置为安全清理模式
     */
    void configure_sanitized() {
        std::lock_guard<std::mutex> lock(mutex_);
        comment_mode = CommentMode::Remove;
    }
};

}  // namespace hps