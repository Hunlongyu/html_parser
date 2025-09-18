#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace hps {
class Element;

class ElementQuery {
  public:
    /**
     * @brief 默认构造函数 Default constructor
     */
    ElementQuery() = default;

    /**
     * @brief 从单个元素构造 ElementQuery Construct ElementQuery from a single element
     * @param element 元素引用 Element reference
     */
    explicit ElementQuery(const Element& element);

    /**
     * @brief 从共享指针元素构造 ElementQuery Construct ElementQuery from shared_ptr element
     * @param element 元素共享指针 Element shared pointer
     */
    explicit ElementQuery(const std::shared_ptr<const Element>& element);

    /**
     * @brief 从元素向量构造 ElementQuery（移动语义）Construct ElementQuery from element vector (move semantics)
     * @param elements 元素向量 Element vector
     */
    explicit ElementQuery(std::vector<std::shared_ptr<const Element>>&& elements);

    /**
     * @brief 从元素向量构造 ElementQuery（拷贝语义）Construct ElementQuery from element vector (copy semantics)
     * @param elements 元素向量 Element vector
     */
    explicit ElementQuery(const std::vector<std::shared_ptr<const Element>>& elements);

    /**
     * @brief 容量查询
     * @return 容量
     */
    [[nodiscard]] size_t size() const noexcept;

    /**
     * @brief 判断是否为空
     * @return 是否为空
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief 元素访问
     * @return 所有元素
     */
    [[nodiscard]] const std::vector<std::shared_ptr<const Element>>& elements() const;

    /**
     * @brief 元素访问
     * @return 第一个元素
     */
    [[nodiscard]] std::shared_ptr<const Element> first_element() const;

    /**
     * @brief 元素访问
     * @return 最后一个元素
     */
    [[nodiscard]] std::shared_ptr<const Element> last_element() const;

    /**
     * @brief 元素访问
     * @param index 索引
     * @return 指定索引的元素
     */
    [[nodiscard]] std::shared_ptr<const Element> operator[](size_t index) const;

    /**
     * @brief 元素访问
     * @param index 索引
     * @return 指定索引的元素
     */
    [[nodiscard]] std::shared_ptr<const Element> at(size_t index) const;

    // 迭代器支持 Iterator support
    using iterator       = std::vector<std::shared_ptr<const Element>>::iterator;
    using const_iterator = std::vector<std::shared_ptr<const Element>>::const_iterator;

    /**
     * @brief 获取开始迭代器 Get begin iterator
     * @return 开始迭代器 Begin iterator
     */
    iterator begin();

    /**
     * @brief 获取结束迭代器 Get end iterator
     * @return 结束迭代器 End iterator
     */
    iterator end();

    /**
     * @brief 获取常量开始迭代器 Get const begin iterator
     * @return 常量开始迭代器 Const begin iterator
     */
    [[nodiscard]] const_iterator begin() const;

    /**
     * @brief 获取常量结束迭代器 Get const end iterator
     * @return 常量结束迭代器 Const end iterator
     */
    [[nodiscard]] const_iterator end() const;

    /**
     * @brief 获取常量开始迭代器 Get const begin iterator
     * @return 常量开始迭代器 Const begin iterator
     */
    [[nodiscard]] const_iterator cbegin() const;

    /**
     * @brief 获取常量结束迭代器 Get const end iterator
     * @return 常量结束迭代器 Const end iterator
     */
    [[nodiscard]] const_iterator cend() const;

    // 条件过滤方法 (返回新的ElementQuery) Conditional filtering methods (return new ElementQuery)

    /**
     * @brief 过滤具有指定属性的元素 Filter elements with specified attribute
     * @param name 属性名 Attribute name
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery has_attribute(std::string_view name) const;

    /**
     * @brief 过滤具有指定属性和值的元素 Filter elements with specified attribute and value
     * @param name 属性名 Attribute name
     * @param value 属性值 Attribute value
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery has_attribute(std::string_view name, std::string_view value) const;

    /**
     * @brief 过滤具有指定 CSS 类的元素 Filter elements with specified CSS class
     * @param class_name 类名 Class name
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery has_class(std::string_view class_name) const;

    /**
     * @brief 过滤具有指定标签名的元素 Filter elements with specified tag name
     * @param tag_name 标签名 Tag name
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery has_tag(std::string_view tag_name) const;

    /**
     * @brief 过滤具有指定文本内容的元素 Filter elements with specified text content
     * @param text 文本内容 Text content
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery has_text(std::string_view text) const;

    /**
     * @brief 过滤包含指定文本的元素 Filter elements containing specified text
     * @param text 文本内容 Text content
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery containing_text(std::string_view text) const;

    /**
     * @brief 过滤文本匹配指定谓词的元素 Filter elements whose text matches specified predicate
     * @param predicate 谓词函数 Predicate function
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery matching_text(const std::function<bool(std::string_view)>& predicate) const;

    // 索引和范围操作 Index and range operations

    /**
     * @brief 获取指定范围的元素切片 Get element slice of specified range
     * @param start 开始索引 Start index
     * @param end 结束索引 End index
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery slice(size_t start, size_t end) const;

    /**
     * @brief 获取前 n 个元素 Get first n elements
     * @param n 元素数量 Number of elements
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery first(size_t n) const;

    /**
     * @brief 获取后 n 个元素 Get last n elements
     * @param n 元素数量 Number of elements
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery last(size_t n) const;

    /**
     * @brief 跳过前 n 个元素 Skip first n elements
     * @param n 跳过的元素数量 Number of elements to skip
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery skip(size_t n) const;

    /**
     * @brief 限制元素数量 Limit number of elements
     * @param n 最大元素数量 Maximum number of elements
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery limit(size_t n) const;

    // 导航方法 Navigation methods

    /**
     * @brief 获取所有子元素 Get all child elements
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery children() const;

    /**
     * @brief 获取匹配选择器的子元素 Get child elements matching selector
     * @param selector CSS 选择器 CSS selector
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery children(std::string_view selector) const;

    /**
     * @brief 获取父元素 Get parent elements
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery parent() const;

    /**
     * @brief 获取所有祖先元素 Get all ancestor elements
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery parents() const;

    /**
     * @brief 获取最近的匹配选择器的祖先元素 Get closest ancestor element matching selector
     * @param selector CSS 选择器 CSS selector
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery closest(std::string_view selector) const;

    /**
     * @brief 获取下一个兄弟元素 Get next sibling element
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery next_sibling() const;

    /**
     * @brief 获取所有后续兄弟元素 Get all following sibling elements
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery next_siblings() const;

    /**
     * @brief 获取上一个兄弟元素 Get previous sibling element
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery prev_sibling() const;

    /**
     * @brief 获取所有前面的兄弟元素 Get all preceding sibling elements
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery prev_siblings() const;

    /**
     * @brief 获取所有兄弟元素 Get all sibling elements
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery siblings() const;

    // CSS 查询方法 CSS query methods

    /**
     * @brief 使用 CSS 选择器查询元素 Query elements using CSS selector
     * @param selector CSS 选择器 CSS selector
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery css(std::string_view selector) const;

    // XPath 查询方法 XPath query methods

    /**
     * @brief 使用 XPath 表达式查询元素 Query elements using XPath expression
     * @param expression XPath 表达式 XPath expression
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery xpath(std::string_view expression) const;

    // 高级查询方法 Advanced query methods

    /**
     * @brief 使用自定义谓词过滤元素 Filter elements using custom predicate
     * @param predicate 谓词函数 Predicate function
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery filter(const std::function<bool(const Element&)>& predicate) const;

    /**
     * @brief 排除匹配选择器的元素 Exclude elements matching selector
     * @param selector CSS 选择器 CSS selector
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery not_(std::string_view selector) const;

    /**
     * @brief 获取偶数索引的元素 Get elements at even indices
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery even() const;

    /**
     * @brief 获取奇数索引的元素 Get elements at odd indices
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery odd() const;

    /**
     * @brief 获取指定索引的元素 Get element at specified index
     * @param index 索引 Index
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery eq(size_t index) const;

    /**
     * @brief 获取索引大于指定值的元素 Get elements with index greater than specified value
     * @param index 索引 Index
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery gt(size_t index) const;

    /**
     * @brief 获取索引小于指定值的元素 Get elements with index less than specified value
     * @param index 索引 Index
     * @return 新的 ElementQuery New ElementQuery
     */
    [[nodiscard]] ElementQuery lt(size_t index) const;

    // 聚合方法 Aggregation methods

    /**
     * @brief 提取所有元素的指定属性值 Extract specified attribute values from all elements
     * @param attr_name 属性名 Attribute name
     * @return 属性值向量 Vector of attribute values
     */
    [[nodiscard]] std::vector<std::string> extract_attributes(std::string_view attr_name) const;

    /**
     * @brief 提取所有元素的文本内容 Extract text content from all elements
     * @return 文本内容向量 Vector of text contents
     */
    [[nodiscard]] std::vector<std::string> extract_texts() const;

    /**
     * @brief 提取所有元素的自有文本内容 Extract own text content from all elements
     * @return 自有文本内容向量 Vector of own text contents
     */
    [[nodiscard]] std::vector<std::string> extract_own_texts() const;

    /**
     * @brief 对每个元素应用映射函数并收集结果 Apply mapping function to each element and collect results
     * @tparam T 结果类型 Result type
     * @param mapper 映射函数 Mapping function
     * @return 映射结果向量 Vector of mapping results
     */
    template <typename T>
    std::vector<T> map(std::function<T(const Element&)> mapper) const;

    // 遍历方法 Traversal methods

    /**
     * @brief 遍历每个元素并执行回调函数 Iterate over each element and execute callback function
     * @param callback 回调函数 Callback function
     * @return 当前 ElementQuery Current ElementQuery
     */
    ElementQuery each(const std::function<void(const Element&)>& callback) const;

    /**
     * @brief 遍历每个元素并执行带索引的回调函数 Iterate over each element and execute callback function with index
     * @param callback 回调函数 Callback function
     * @return 当前 ElementQuery Current ElementQuery
     */
    ElementQuery each(const std::function<void(size_t, const Element&)>& callback) const;

    // 布尔检查方法 Boolean check methods

    /**
     * @brief 检查是否有元素匹配选择器 Check if any element matches selector
     * @param selector CSS 选择器 CSS selector
     * @return 是否匹配 Whether matches
     */
    [[nodiscard]] bool is(std::string_view selector) const;

    /**
     * @brief 检查是否包含指定元素 Check if contains specified element
     * @param element 元素 Element
     * @return 是否包含 Whether contains
     */
    [[nodiscard]] bool contains(const Element& element) const;

    /**
     * @brief 检查是否包含指定文本 Check if contains specified text
     * @param text 文本内容 Text content
     * @return 是否包含 Whether contains
     */
    [[nodiscard]] bool contains(std::string_view text) const;

  private:
    std::vector<std::shared_ptr<const Element>> m_elements;
};

// 模板方法实现 Template method implementation
template <typename T>
std::vector<T> ElementQuery::map(std::function<T(const Element&)> mapper) const {
    std::vector<T> results;
    results.reserve(m_elements.size());

    for (const auto& element : m_elements) {
        if (element) {
            results.push_back(mapper(*element));
        }
    }

    return results;
}

}  // namespace hps