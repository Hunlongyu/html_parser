#pragma once

#include <algorithm>
#include <array>
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
 * @brief 文本处理模式枚举
 *
 * 定义如何处理HTML文本内容中的实体和特殊标签。
 */
enum class TextProcessingMode {
    Raw,     ///< 保持原始文本，不进行任何转换
    Decode,  ///< 解码HTML实体但保留标签
};

/**
 * @brief <br> 处理模式枚举
 *
 * 定义如何处理HTML中的<br>元素。
 */
enum class BRHandling {
    Keep,           ///< 保持 <br> 元素，不额外插入文本
    InsertNewline,  ///< 在 <br> 处插入换行符 "\n"
    InsertCustom    ///< 在 <br> 处插入自定义文本（由 br_text 指定）
};

/**
 * @brief HTML解析器配置选项类
 *
 * Options类提供了HTML解析器的各种配置选项，包括错误处理模式、
 * 内容处理选项、性能限制和自定义过滤器等。支持通过工厂方法
 * 创建预定义的配置组合。
 */

class Options {
  public:
    /**
     * @brief 默认构造函数
     *
     * 创建具有默认配置的Options实例。
     */
    Options() = default;

    /**
     * @brief 拷贝构造函数
     */
    Options(const Options&) = default;

    /**
     * @brief 移动构造函数
     */
    Options(Options&&) = default;

    /**
     * @brief 拷贝赋值操作符
     */
    Options& operator=(const Options&) = default;

    /**
     * @brief 移动赋值操作符
     */
    Options& operator=(Options&&) = default;

    /**
     * @brief 析构函数
     */
    ~Options() = default;

    // ==================== 工厂方法 ====================

    /**
     * @brief 创建严格模式配置
     *
     * 严格模式下，解析器会对HTML格式要求更严格，
     * 遇到错误时立即抛出异常，适用于需要高质量HTML的场景。
     *
     * @return 配置为严格模式的Options实例
     */
    static Options strict() {
        Options opts;
        opts.error_handling = ErrorHandlingMode::Strict;
        opts.max_tokens     = 100000;
        opts.max_depth      = 100;
        opts.max_attributes = 50;
        return opts;
    }

    /**
     * @brief 创建性能优化配置
     *
     * 性能模式下，解析器会移除注释和多余空白，
     * 提高解析速度和内存效率，适用于大量HTML处理场景。
     *
     * @return 配置为性能优化的Options实例
     */
    static Options performance() {
        Options opts;
        opts.comment_mode    = CommentMode::Remove;
        opts.whitespace_mode = WhitespaceMode::Remove;
        opts.max_tokens      = 10000000;
        opts.max_depth       = 2000;
        return opts;
    }

    /**
     * @brief 创建安全清理配置
     *
     * 安全模式下，解析器会移除注释和潜在的安全风险内容，
     * 适用于处理不可信HTML内容的场景。
     *
     * @return 配置为安全清理的Options实例
     */
    static Options sanitized() {
        Options opts;
        opts.comment_mode   = CommentMode::Remove;
        opts.error_handling = ErrorHandlingMode::Lenient;
        // 设置更严格的限制以防止DoS攻击
        opts.max_tokens                 = 500000;
        opts.max_depth                  = 500;
        opts.max_attributes             = 50;
        opts.max_attribute_name_length  = 128;
        opts.max_attribute_value_length = 4096;
        opts.max_text_length            = 524288;  // 512KB
        return opts;
    }

    // ==================== void元素判断方法 ====================

    /**
     * @brief 判断指定标签是否为void元素
     *
     * void元素是HTML中不能包含内容且不需要结束标签的元素，
     * 如<br>、<img>、<input>等。
     *
     * @param tag_name 标签名称
     * @return 如果是void元素返回true，否则返回false
     */
    [[nodiscard]] bool is_void_element(std::string_view tag_name) const {
        if (!void_elements.empty()) {
            return void_elements.contains(std::string(tag_name));
        }

        // 优化的二分查找，避免构建 set
        static constexpr std::array<std::string_view, 20> default_void_tags = {"area", "base", "basefont", "br", "col", "command", "embed", "frame", "hr", "img", "input", "isindex", "keygen", "link", "menuitem", "meta", "param", "source", "track", "wbr"};

        return std::ranges::binary_search(default_void_tags, tag_name);
    }

    /**
     * @brief 获取默认void元素集合
     *
     * 返回HTML5标准定义的void元素列表，包括向后兼容的HTML4元素。
     *
     * @return 默认void元素集合的常量引用
     */
    static const std::unordered_set<std::string>& get_default_void_elements() {
        static const std::unordered_set<std::string> default_void_elements = {
            // HTML5 标准 void 元素（不能有结束标签）
            "area",    ///< 图像映射区域
            "base",    ///< 文档基础URL
            "br",      ///< 换行符
            "col",     ///< 表格列
            "embed",   ///< 嵌入内容
            "hr",      ///< 水平分割线
            "img",     ///< 图像
            "input",   ///< 输入控件
            "link",    ///< 外部资源链接
            "meta",    ///< 元数据
            "param",   ///< 对象参数
            "source",  ///< 媒体资源
            "track",   ///< 文本轨道
            "wbr",     ///< 可选换行点

            // HTML4 遗留 void 元素（向后兼容）
            "basefont",  ///< 基础字体（已废弃但仍需支持）
            "frame",     ///< 框架（已废弃但仍需支持）
            "isindex",   ///< 索引输入（已废弃但仍需支持）

            // 常见的自闭合元素
            "command",   ///< 命令按钮
            "keygen",    ///< 密钥生成器
            "menuitem",  ///< 菜单项
        };
        return default_void_elements;
    }

    // ==================== 配置选项 ====================

    // 核心解析选项
    ErrorHandlingMode error_handling = ErrorHandlingMode::Lenient;  ///< ✅ 错误处理模式，默认宽松模式

    // 内容处理选项
    CommentMode        comment_mode         = CommentMode::Preserve;     ///< ✅ 注释处理模式，默认保留注释
    WhitespaceMode     whitespace_mode      = WhitespaceMode::Preserve;  ///< ✅ 空白文本处理模式，默认保留空白
    TextProcessingMode text_processing_mode = TextProcessingMode::Raw;   ///< ✅ 文本处理模式，默认保持原始
    BRHandling         br_handling          = BRHandling::Keep;          ///< <br> 处理策略
    std::string        br_text              = "\n";                      ///< 自定义文本（InsertCustom 时使用）

    // 高级选项
    bool preserve_case = false;  ///< ✅ 是否保持标签和属性名大小写，默认转为小写

    // 性能和安全限制
    size_t max_tokens                 = 1000000;  ///< 最大Token数量限制
    size_t max_depth                  = 1000;     ///< 最大嵌套深度限制
    size_t max_attributes             = 100;      ///< 单个元素最大属性数限制
    size_t max_attribute_name_length  = 256;      ///< 属性名最大长度限制
    size_t max_attribute_value_length = 8192;     ///< 属性值最大长度限制
    size_t max_text_length            = 1048576;  ///< 文本节点最大长度限制（1MB）

    // CSS 解析器配置
    size_t max_css3_cache_size = 1000;  ///< 最大缓存条目数量

    // 自定义配置
    std::unordered_set<std::string> void_elements;  ///< ✅ 自定义void元素列表，为空时使用默认列表

    // ==================== 便利方法 ====================

    /**
     * @brief 添加自定义void元素
     *
     * @param element 要添加的void元素名
     */
    void add_void_element(const std::string& element) {
        void_elements.insert(element);
    }

    /**
     * @brief 移除自定义void元素
     *
     * @param element 要移除的void元素名
     */
    void remove_void_element(const std::string& element) {
        void_elements.erase(element);
    }

    /**
     * @brief 清空自定义void元素列表
     *
     * 清空后将使用默认的void元素列表。
     */
    void clear_void_elements() {
        void_elements.clear();
    }

    /**
     * @brief 重置所有配置为默认值
     */
    void reset_to_defaults() {
        *this = Options{};
    }

    /**
     * @brief 检查配置是否有效
     *
     * @return 如果配置有效返回true，否则返回false
     */
    [[nodiscard]] bool is_valid() const noexcept {
        return max_tokens > 0 && max_depth > 0 && max_attributes > 0 && max_attribute_name_length > 0 && max_attribute_value_length > 0 && max_text_length > 0;
    }
};

}  // namespace hps
