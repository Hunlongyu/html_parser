#pragma once

#include "hps/core/element.hpp"
#include "hps/parsing/options.hpp"
#include "hps/utils/exception.hpp"
#include "hps/utils/noncopyable.hpp"

namespace hps {

/**
 * @brief HTML树构建器类
 *
 * TreeBuilder负责将Token序列转换为DOM树结构。它维护一个元素栈来跟踪
 * 当前的嵌套结构，并处理各种HTML标签的开始、结束和文本内容。
 *
 * 该类继承自NonCopyable，确保树构建器实例不能被复制，避免状态混乱。
 */
class TreeBuilder : public NonCopyable {
  public:
    /**
     * @brief 构造函数
     * @param document 目标文档对象的智能指针，用于存储构建的 DOM 树
     * @param options 解析选项
     */
    explicit TreeBuilder(const std::shared_ptr<Document>& document, const Options& options);

    /**
     * @brief 析构函数
     *
     * 使用默认析构函数，智能指针会自动管理资源释放
     */
    ~TreeBuilder() = default;

    // === 核心处理功能 ===

    /**
     * @brief 处理单个Token
     * @param token 要处理的Token对象
     * @return 是否成功处理Token
     *
     * 根据Token类型分发到相应的处理方法：
     * - 开始标签：调用process_start_tag
     * - 结束标签：调用process_end_tag
     * - 文本内容：调用process_text
     */
    [[nodiscard]] bool process_token(const Token& token);

    /**
     * @brief 完成树构建过程
     * @return 是否成功完成构建
     *
     * 执行最终的清理工作，确保所有未闭合的标签得到正确处理，
     * 并验证DOM树的完整性。
     */
    [[nodiscard]] bool finish();

    // === 访问器方法 ===

    /**
     * @brief 获取构建的文档对象
     * @return 文档对象的智能指针
     *
     * 返回包含完整DOM树的文档对象，通常在树构建完成后调用。
     */
    [[nodiscard]] std::shared_ptr<Document> document() const noexcept;

    /**
     * @brief 获取解析过程中的错误列表
     * @return 解析错误列表的常量引用
     *
     * 返回在解析过程中收集的所有错误信息，用于诊断和调试。
     */
    [[nodiscard]] const std::vector<HPSError>& errors() const noexcept;

  private:
    // === Token处理方法 ===

    /**
     * @brief 处理开始标签Token
     * @param token 开始标签Token
     *
     * 创建新元素并将其添加到DOM树中，同时推入元素栈以跟踪嵌套结构。
     */
    void process_start_tag(const Token& token);

    /**
     * @brief 处理结束标签Token
     * @param token 结束标签Token
     *
     * 查找匹配的开始标签并从元素栈中弹出相应元素，处理标签不匹配的情况。
     */
    void process_end_tag(const Token& token);

    /**
     * @brief 处理文本内容Token
     * @param token 文本Token
     *
     * 将文本内容添加到当前元素中，处理空白字符的规范化。
     */
    void process_text(const Token& token) const;

    /**
     * @brief 处理注释内容 Token
     * @param token 注释 Token
     *
     * 将注释内容添加到当前元素中，处理空白字符的规范化。
     */
    void process_comment(const Token& token) const;

    // === 元素操作方法 ===

    /**
     * @brief 根据Token创建Element对象
     * @param token 包含标签信息的Token
     * @return 新创建的Element智能指针
     *
     * 静态方法，根据Token中的标签名和属性创建相应的Element对象。
     */
    [[nodiscard]] static std::shared_ptr<Element> create_element(const Token& token);

    /**
     * @brief 将元素插入到DOM树中
     * @param element 要插入的元素
     *
     * 将元素作为当前元素的子节点插入到DOM树的适当位置。
     */
    void insert_element(const std::shared_ptr<Element>& element) const;

    /**
     * @brief 插入文本节点
     * @param text 要插入的文本内容
     *
     * 在当前元素下创建并插入文本节点。
     */
    void insert_text(std::string_view text) const;

    /**
     * @brief 插入注释节点
     * @param comment 要插入的注释内容
     *
     * 在当前元素下创建并插入注释节点。
     */
    void insert_comment(std::string_view comment) const;

    // === 栈操作方法 ===

    /**
     * @brief 将元素推入元素栈
     * @param element 要推入的元素
     *
     * 将元素添加到栈顶，成为新的当前元素。
     */
    void push_element(const std::shared_ptr<Element>& element);

    /**
     * @brief 从元素栈弹出顶部元素
     * @return 被弹出的元素，如果栈为空则返回nullptr
     *
     * 移除并返回栈顶元素，用于处理结束标签。
     */
    [[nodiscard]] std::shared_ptr<Element> pop_element();

    /**
     * @brief 获取当前元素（栈顶元素）
     * @return 当前元素的智能指针，如果栈为空则返回nullptr
     *
     * 返回栈顶元素但不移除它，用于确定新节点的插入位置。
     */
    [[nodiscard]] std::shared_ptr<Element> current_element() const;

    // === 作用域和验证方法 ===

    /**
     * @brief 检查是否应该关闭指定标签
     * @param tag_name 标签名称
     * @return 如果应该关闭则返回true
     *
     * 简单的作用域检查，用于处理HTML的容错性。
     */
    [[nodiscard]] bool should_close_element(std::string_view tag_name) const;

    /**
     * @brief 关闭元素直到遇到指定标签
     * @param tag_name 目标标签名称
     *
     * 从栈顶开始弹出元素，直到找到匹配的标签或栈为空。
     * 用于处理不正确嵌套的HTML标签。
     */
    void close_elements_until(std::string_view tag_name);

    // === 错误处理方法 ===

    /**
     * @brief 记录解析错误
     * @param code 错误代码
     * @param message 错误描述信息
     *
     * 将解析错误添加到错误列表中，用于后续的错误报告和调试。
     */
    void parse_error(ErrorCode code, const std::string& message);

  private:
    std::shared_ptr<Document>             m_document;       ///< 目标文档对象，存储构建的DOM树
    std::vector<std::shared_ptr<Element>> m_element_stack;  ///< 元素栈，跟踪当前的嵌套结构
    std::vector<HPSError>                 m_errors;         ///< 解析错误列表，收集处理过程中的错误
    const Options&                        m_options;        ///< 解析选项
};

}  // namespace hps