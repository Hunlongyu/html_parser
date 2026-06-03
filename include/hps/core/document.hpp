#pragma once
#include "hps/core/node.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace hps {

class ElementQuery;

/**
 * @brief HTML 文档类
 *
 * Document 类表示一个完整的 HTML 文档，继承自 Node 类。
 * 它提供了访问文档结构、元数据和执行查询操作的功能。
 * Document 是 DOM 树的根节点，包含整个 HTML 文档的结构信息。
 */
class Document : public Node {
  public:
    /**
     * @brief 构造函数
     * @param html_content HTML 源代码字符串
     * @param base_url 文档来源 URL（页面地址），用于相对链接解析；可为空
     */
    explicit Document(std::string html_content, std::string base_url = {});

    /**
     * @brief 虚析构函数
     */
    ~Document() override = default;

    // Node Interface Overrides
    /**
     * @brief 获取节点类型
     * @return 节点类型，对于 Document 始终返回 NodeType::Document
     */
    [[nodiscard]] NodeType type() const noexcept override;

    /**
     * @brief 获取文档的文本内容
     * @return 文档中所有文本节点的内容拼接而成的字符串
     */
    [[nodiscard]] std::string text_content() const override;

    // Document Metadata Access
    /**
     * @brief 获取文档标题
     * @return 文档的 title 元素内容，如果不存在则返回空字符串
     */
    [[nodiscard]] std::string title() const;

    /**
     * @brief 获取文档字符编码
     * @return 文档的字符编码（从 meta charset 或 Content-Type 获取），如果未指定则返回空字符串
     */
    [[nodiscard]] std::string charset() const;

    /**
     * @brief 获取原始 HTML 源代码
     * @return 构造文档时传入的原始 HTML 字符串
     */
    [[nodiscard]] std::string_view source_html() const noexcept;

    /**
     * @brief 将整个文档树序列化为 HTML
     *
     * 反映解析后的 DOM（可能与 source_html 不同：如编码已转码、容错已修复）；
     * 不包含 DOCTYPE（本库不将其表示为节点）。
     * @return 文档全部子节点序列化而成的 HTML 字符串
     */
    [[nodiscard]] std::string outer_html() const;

    // Meta Information Extraction
    /**
     * @brief 获取指定 name 属性的 meta 标签内容
     * @param name meta 标签的 name 属性值（例如 "description", "keywords"）
     * @return meta 标签的 content 属性值，如果不存在则返回空字符串
     */
    [[nodiscard]] std::string get_meta_content(std::string_view name) const;

    /**
     * @brief 获取指定 property 属性的 meta 标签内容
     * @param property meta 标签的 property 属性值（例如 "og:title", "og:description"）
     * @return meta 标签的 content 属性值，如果不存在则返回空字符串
     */
    [[nodiscard]] std::string get_meta_property(std::string_view property) const;

    // URL Resolution
    /**
     * @brief 获取文档的有效基准 URL（用于相对链接解析）
     * @return 若存在 `<base href>` 则返回其相对 Options::base_url 解析后的结果，否则返回 Options::base_url；均未提供时为空串
     */
    [[nodiscard]] const std::string& base_url() const;

    /**
     * @brief 将一个（可能是相对的）引用解析为绝对 URL
     * @param reference href/src 等引用
     * @return 基于 base_url() 解析的结果（base_url 为空时原样返回 reference）
     */
    [[nodiscard]] std::string resolve_url(std::string_view reference) const;

    // Resource Extraction
    /**
     * @brief 获取文档中所有链接
     * @return 所有 a[href] 的 href；当已知 base_url() 时解析为绝对地址，否则为原始值
     */
    [[nodiscard]] std::vector<std::string> get_all_links() const;

    /**
     * @brief 获取文档中所有图片链接
     * @return 所有 img[src] 的 src；当已知 base_url() 时解析为绝对地址，否则为原始值
     */
    [[nodiscard]] std::vector<std::string> get_all_images() const;

    // Document Structure Access
    /**
     * @brief 获取文档根元素
     * @return 文档的根元素（通常是 html 元素），如果不存在则返回 nullptr
     */
    [[nodiscard]] const Element* root() const;

    /**
     * @brief 获取 HTML 元素
     * @return 文档的 html 元素，如果不存在则返回 nullptr
     */
    [[nodiscard]] const Element* html() const;

    // Element Query Methods
    /**
     * @brief 使用 CSS 选择器查找第一个匹配的元素
     * @param selector CSS 选择器字符串（例如 "#id", ".class", "tag"）
     * @return 第一个匹配的元素，如果没有找到则返回 nullptr
     */
    [[nodiscard]] const Element* query_selector(std::string_view selector) const;

    /**
     * @brief 使用 CSS 选择器查找所有匹配的元素
     * @param selector CSS 选择器字符串
     * @return 包含所有匹配元素的向量，如果没有找到则返回空向量
     */
    [[nodiscard]] std::vector<const Element*> query_selector_all(std::string_view selector) const;

    /**
     * @brief 根据 ID 查找元素
     * @param id 元素的 id 属性值
     * @return 具有指定 ID 的元素，如果不存在则返回 nullptr
     */
    [[nodiscard]] const Element* get_element_by_id(std::string_view id) const;

    /**
     * @brief 根据标签名查找所有元素
     * @param tag_name 标签名（例如 "div", "p", "a"）
     * @return 包含所有指定标签名元素的向量，如果没有找到则返回空向量
     */
    [[nodiscard]] std::vector<const Element*> get_elements_by_tag_name(std::string_view tag_name) const;

    /**
     * @brief 根据 CSS 类名查找所有元素
     * @param class_name CSS 类名
     * @return 包含所有具有指定类名元素的向量，如果没有找到则返回空向量
     */
    [[nodiscard]] std::vector<const Element*> get_elements_by_class_name(std::string_view class_name) const;

    // Advanced Query Methods
    /**
     * @brief 创建 CSS 选择器查询对象
     * @param selector CSS 选择器字符串
     * @return ElementQuery 对象，用于链式查询操作
     */
    [[nodiscard]] ElementQuery css(std::string_view selector) const;

    // Document Modification
    /**
     * @brief 向文档添加子节点
     * @param child 要添加的子节点
     *
     * 将指定的节点添加为文档的子节点。通常用于添加根元素（如 html 元素）。
     * 如果传入的节点为 nullptr，则不执行任何操作。
     */
    Node* add_child(std::unique_ptr<Node> child);
    Node* insert_child_before(std::unique_ptr<Node> child, const Node* before);

    /**
     * @brief 取出并移除所有直接子节点
     * @return 当前文档原有子节点的所有权数组
     */
    std::vector<std::unique_ptr<Node>> take_children();

  private:
    struct QueryIndexCache {
        std::unordered_map<std::string, const Element*>              id_lookup;
        std::unordered_map<std::string, std::vector<const Element*>> class_lookup;
        std::unordered_map<std::string, std::vector<const Element*>> tag_lookup;
        bool                                                         valid{false};
    };

    void invalidate_query_indexes() noexcept;
    void ensure_query_indexes() const;
    void index_element_subtree(const Element& element) const;

    std::string m_html_source; /**< 原始 HTML 源代码 */
    std::string m_base_url;     /**< 文档来源 URL（页面地址），来自 Options::base_url */

    mutable QueryIndexCache            m_query_index_cache;
    mutable std::optional<std::string> m_cached_title;    /**< 缓存的文档标题 */
    mutable std::optional<std::string> m_cached_charset;  /**< 缓存的字符编码 */
    mutable std::optional<std::string> m_cached_base_url; /**< 缓存的有效基准 URL（含 <base href>） */

    friend class Node;
};

}  // namespace hps
