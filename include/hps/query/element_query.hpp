#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace hps {
class Element;

class ElementQuery {
  public:
    ElementQuery() = default;
    explicit ElementQuery(std::vector<std::shared_ptr<const Element>>&& elements);
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

    // 迭代器支持
    using iterator       = std::vector<std::shared_ptr<const Element>>::iterator;
    using const_iterator = std::vector<std::shared_ptr<const Element>>::const_iterator;
    iterator       begin();
    iterator       end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;

    // 条件过滤方法 (返回新的ElementQuery)
    [[nodiscard]] ElementQuery has_attribute(std::string_view name) const;
    [[nodiscard]] ElementQuery has_attribute(std::string_view name, std::string_view value) const;
    [[nodiscard]] ElementQuery has_class(std::string_view class_name) const;
    [[nodiscard]] ElementQuery has_tag(std::string_view tag_name) const;
    [[nodiscard]] ElementQuery has_text(std::string_view text) const;
    [[nodiscard]] ElementQuery containing_text(std::string_view text) const;
    [[nodiscard]] ElementQuery matching_text(std::function<bool(std::string_view)> predicate) const;

    // 索引和范围操作
    [[nodiscard]] ElementQuery slice(size_t start, size_t end) const;
    [[nodiscard]] ElementQuery first(size_t n) const;
    [[nodiscard]] ElementQuery last(size_t n) const;
    [[nodiscard]] ElementQuery skip(size_t n) const;
    [[nodiscard]] ElementQuery limit(size_t n) const;

    // 导航方法
    [[nodiscard]] ElementQuery children() const;
    [[nodiscard]] ElementQuery children(std::string_view selector) const;
    [[nodiscard]] ElementQuery parent() const;
    [[nodiscard]] ElementQuery parents() const;
    [[nodiscard]] ElementQuery closest(std::string_view selector) const;
    [[nodiscard]] ElementQuery next_sibling() const;
    [[nodiscard]] ElementQuery next_siblings() const;
    [[nodiscard]] ElementQuery prev_sibling() const;
    [[nodiscard]] ElementQuery prev_siblings() const;
    [[nodiscard]] ElementQuery siblings() const;

    // CSS 查询方法
    [[nodiscard]] ElementQuery css(std::string_view selector) const;
    // XPath 查询方法
    [[nodiscard]] ElementQuery xpath(std::string_view expression) const;

    // 高级查询方法
    [[nodiscard]] ElementQuery filter(std::function<bool(const Element&)> predicate) const;
    [[nodiscard]] ElementQuery not_(std::string_view selector) const;
    [[nodiscard]] ElementQuery even() const;
    [[nodiscard]] ElementQuery odd() const;
    [[nodiscard]] ElementQuery eq(size_t index) const;
    [[nodiscard]] ElementQuery gt(size_t index) const;
    [[nodiscard]] ElementQuery lt(size_t index) const;

    // 聚合方法
    [[nodiscard]] std::vector<std::string> extract_attributes(std::string_view attr_name) const;
    [[nodiscard]] std::vector<std::string> extract_texts() const;
    [[nodiscard]] std::vector<std::string> extract_own_texts() const;
    template <typename T>
    std::vector<T> map(std::function<T(const Element&)> mapper) const;

    // 遍历方法
    [[nodiscard]] ElementQuery each(std::function<void(const Element&)> callback) const;
    [[nodiscard]] ElementQuery each(std::function<void(size_t, const Element&)> callback) const;

    // 布尔检查方法
    [[nodiscard]] bool is(std::string_view selector) const;
    [[nodiscard]] bool contains(const Element& element) const;
    [[nodiscard]] bool contains(std::string_view text) const;

  private:
    std::vector<std::shared_ptr<const Element>> m_elements;
};

// 模板方法实现
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