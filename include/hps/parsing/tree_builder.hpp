#pragma once

#include "hps/core/element.hpp"
#include "hps/parsing/options.hpp"
#include "hps/utils/exception.hpp"
#include "hps/utils/noncopyable.hpp"

#include <utility>

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
    TreeBuilder(const std::shared_ptr<Document>& document, const Options& options, Element* fragment_context);

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
    [[nodiscard]] bool process_token(const Token& token, size_t position = 0);

    /**
     * @brief 完成树构建过程
     * @return 是否成功完成构建
     *
     * 执行最终的清理工作，确保所有未闭合的标签得到正确处理，
     * 并验证DOM树的完整性。
     */
    [[nodiscard]] bool finish();

    /**
     * @brief 获取解析过程中的错误列表
     * @return 解析错误列表的常量引用
     *
     * 返回在解析过程中收集的所有错误信息，用于诊断和调试。
     */
    [[nodiscard]] const std::vector<HPSError>& errors() const noexcept;

    /**
     * @brief 获取并消耗解析过程中的错误列表（移动语义）
     * @return 解析错误列表
     *
     * 调用此方法后，TreeBuilder内部的错误列表将被清空。
     * 适用于需要将错误信息转移到其他地方（如HTMLParser）的场景。
     */
    [[nodiscard]] std::vector<HPSError> consume_errors();

  private:
    // === Token处理方法 ===

    /**
     * @brief 处理开始标签Token
     * @param token 开始标签Token
     *
     * 创建新元素并将其添加到DOM树中，同时推入元素栈以跟踪嵌套结构。
     */
    void process_start_tag(const Token& token);
    void process_html_start_tag(const Token& token);
    void process_head_start_tag(const Token& token);
    void process_body_start_tag(const Token& token);

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
    void process_text(const Token& token);

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
    [[nodiscard]] static std::unique_ptr<Element> create_element(const Token& token);
    [[nodiscard]] static std::unique_ptr<Element> create_element(
        const Token& token,
        NamespaceKind namespace_kind);
    static void merge_token_attributes(Element& element, const Token& token);

    /**
     * @brief 将元素插入到DOM树中
     * @param element 要插入的元素
     *
     * 将元素作为当前元素的子节点插入到DOM树的适当位置。
     */
    void insert_element(std::unique_ptr<Element> element) const;

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
    void push_element(Element* element);
    void push_if_absent(Element* element);

    /**
     * @brief 获取当前元素（栈顶元素）
     * @return 当前元素的原始指针，如果栈为空则返回nullptr
     *
     * 返回栈顶元素但不移除它，用于确定新节点的插入位置。
     */
    [[nodiscard]] Element* current_element() const;
    [[nodiscard]] bool is_on_stack(const Element* element) const noexcept;

    /**
     * @brief 关闭元素直到遇到指定标签
     * @param tag_name 目标标签名称
     *
     * 从栈顶开始弹出元素，直到找到匹配的标签或栈为空。
     * 用于处理不正确嵌套的HTML标签。
     */
    void close_elements_until(std::string_view tag_name, bool report_auto_close_errors = true);

    /**
     * @brief 检查并处理隐含关闭的标签
     * @param tag_name 当前遇到的开始标签名称
     *
     * 如果当前栈顶元素是可以被新标签隐含关闭的（如 <p> 遇到 <p>），则自动关闭栈顶元素。
     */
    void check_implicit_close(std::string_view tag_name);
    void ensure_html_element();
    void ensure_head_element();
    void ensure_body_element();
    void close_head_element_if_open();
    void prepare_table_context_for_start_tag(std::string_view tag_name);
    void prepare_select_context_for_start_tag(std::string_view tag_name);
    [[nodiscard]] bool handle_table_end_tag(std::string_view tag_name);
    [[nodiscard]] bool handle_select_end_tag(std::string_view tag_name);
    void close_open_table_content_before_container(std::string_view tag_name, bool close_matching_tag = true);
    void ensure_table_section(std::string_view tag_name = "tbody");
    void ensure_table_row();
    void ensure_colgroup();
    void close_colgroup_for_non_col_token();
    Node* insert_node(std::unique_ptr<Node> child, Node* parent) const;
    Node* insert_node_before(std::unique_ptr<Node> child, Node* parent, const Node* before) const;
    void insert_text_before(std::string_view text, Node* parent, const Node* before) const;

    [[nodiscard]] static bool is_head_content_tag(std::string_view tag_name) noexcept;
    [[nodiscard]] static bool is_table_section_tag(std::string_view tag_name) noexcept;
    [[nodiscard]] static bool is_table_cell_tag(std::string_view tag_name) noexcept;
    [[nodiscard]] static bool is_table_structure_tag(std::string_view tag_name) noexcept;
    [[nodiscard]] static bool is_table_container_tag(std::string_view tag_name) noexcept;
    [[nodiscard]] static bool is_adoption_formatting_tag(std::string_view tag_name) noexcept;
    [[nodiscard]] NamespaceKind current_insertion_namespace() const noexcept;
    [[nodiscard]] NamespaceKind namespace_for_start_tag(std::string_view tag_name) const noexcept;
    [[nodiscard]] static bool can_omit_end_tag_at_eof(std::string_view tag_name) noexcept;
    [[nodiscard]] static bool is_all_whitespace(std::string_view text) noexcept;
    [[nodiscard]] bool should_foster_parent_text() const noexcept;
    [[nodiscard]] bool should_foster_parent_element(std::string_view tag_name) const noexcept;
    [[nodiscard]] std::pair<Node*, const Node*> foster_parent_insertion_point() const noexcept;
    void close_foster_parented_elements_before_table_token() noexcept;
    [[nodiscard]] bool try_recover_formatting_end_tag(std::string_view tag_name);
    [[nodiscard]] Element* find_open_element(
        std::string_view tag_name,
        bool include_fragment_base = true) const noexcept;
    [[nodiscard]] Element* find_open_in_select_scope(std::string_view tag_name) const noexcept;
    [[nodiscard]] Element* find_open_table_section() const noexcept;
    [[nodiscard]] Element* find_open_table_row() const noexcept;
    [[nodiscard]] Element* find_open_table_cell() const noexcept;

    // === 错误处理方法 ===

    /**
     * @brief 记录解析错误
     * @param code 错误代码
     * @param message 错误描述信息
     *
     * 将解析错误添加到错误列表中，用于后续的错误报告和调试。
     */
    void parse_error(ErrorCode code, const std::string& message, size_t position = 0);

  private:
    std::shared_ptr<Document> m_document;       ///< 目标文档对象，存储构建的DOM树
    std::vector<Element*>     m_element_stack;  ///< 元素栈，跟踪当前的嵌套结构
    std::vector<std::string>  m_ignored_element_stack;  ///< 超过深度限制后跳过的元素栈
    std::vector<HPSError>     m_errors;         ///< 解析错误列表，收集处理过程中的错误
    const Options&            m_options;        ///< 解析选项
    size_t                    m_last_position = 0;  ///< 最近处理到的源码位置
    Element*                  m_html_element = nullptr;  ///< 文档 html 元素
    Element*                  m_head_element = nullptr;  ///< 文档 head 元素
    Element*                  m_body_element = nullptr;  ///< 文档 body 元素
    Element*                  m_fragment_context = nullptr;  ///< fragment 解析上下文元素
    size_t                    m_stack_floor      = 0;        ///< fragment 栈底，不允许弹出
    bool                      m_head_closed  = false;    ///< head 是否已经结束
};

}  // namespace hps
