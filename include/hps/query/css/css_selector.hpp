#pragma once

#include "hps/utils/string_pool.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace hps {

class Element;

// CSS选择器类型枚举
enum class SelectorType : std::uint8_t {
    Universal,     // *
    Type,          // div, p, span
    Class,         // .class-name
    Id,            // #id-name
    Attribute,     // [attr], [attr=value]
    Descendant,    // div p (空格)
    Child,         // div > p
    Adjacent,      // div + p
    Sibling,       // div ~ p
    Compound,      // div.class#id
    PseudoClass,   // :hover, :first-child
    PseudoElement  // ::before, ::after
};

// 属性选择器操作符
enum class AttributeOperator : std::uint8_t {
    Exists,      // [attr]
    Equals,      // [attr=value]
    Contains,    // [attr*=value]
    StartsWith,  // [attr^=value]
    EndsWith,    // [attr$=value]
    WordMatch,   // [attr~=value]
    LangMatch    // [attr|=value]
};

// 选择器优先级结构
struct SelectorSpecificity {
    int inline_style = 0;  // 内联样式
    int ids          = 0;  // ID选择器数量
    int classes      = 0;  // 类、属性、伪类选择器数量
    int elements     = 0;  // 元素、伪元素选择器数量

    // 比较优先级
    bool operator<(const SelectorSpecificity& other) const {
        if (inline_style != other.inline_style)
            return inline_style < other.inline_style;
        if (ids != other.ids)
            return ids < other.ids;
        if (classes != other.classes)
            return classes < other.classes;
        return elements < other.elements;
    }

    bool operator==(const SelectorSpecificity& other) const {
        return inline_style == other.inline_style && ids == other.ids && classes == other.classes && elements == other.elements;
    }
};

// CSS选择器AST节点基类
class CSSSelector {
  public:
    explicit CSSSelector(const SelectorType type)
        : m_type(type) {}
    virtual ~CSSSelector() = default;

    // 禁用拷贝，只允许移动
    CSSSelector(const CSSSelector&)            = delete;
    CSSSelector& operator=(const CSSSelector&) = delete;
    CSSSelector(CSSSelector&&)                 = default;
    CSSSelector& operator=(CSSSelector&&)      = default;

    [[nodiscard]] SelectorType type() const noexcept {
        return m_type;
    }

    [[nodiscard]] virtual bool                matches(const Element& element) const = 0;
    [[nodiscard]] virtual std::string         to_string() const                     = 0;
    [[nodiscard]] virtual SelectorSpecificity calculate_specificity() const         = 0;

    // 快速匹配检查（可选优化）
    [[nodiscard]] virtual bool can_quick_reject(const Element& element) const {
        return false;
    }

  protected:
    SelectorType m_type;
};

// 通用选择器 *
class UniversalSelector : public CSSSelector {
  public:
    UniversalSelector()
        : CSSSelector(SelectorType::Universal) {}

    [[nodiscard]] bool matches(const Element& element) const override {
        return true;  // 通用选择器匹配所有元素
    }

    [[nodiscard]] std::string to_string() const override {
        return "*";
    }

    [[nodiscard]] SelectorSpecificity calculate_specificity() const override {
        return {.inline_style = 0, .ids = 0, .classes = 0, .elements = 0};  // 通用选择器优先级为0
    }
};

// 类型选择器 div, p, span
class TypeSelector : public CSSSelector {
  public:
    explicit TypeSelector(std::string_view tag_name)
        : CSSSelector(SelectorType::Type),
          m_tag_name(tag_name) {}

    [[nodiscard]] bool matches(const Element& element) const override;

    [[nodiscard]] std::string to_string() const override {
        return std::string(m_tag_name);
    }

    [[nodiscard]] std::string_view tag_name() const noexcept {
        return m_tag_name;
    }

    [[nodiscard]] SelectorSpecificity calculate_specificity() const override {
        return {.inline_style = 0, .ids = 0, .classes = 0, .elements = 1};  // 元素选择器优先级为1
    }

    [[nodiscard]] bool can_quick_reject(const Element& element) const override;

  private:
    std::string_view m_tag_name;
};

// 类选择器 .class-name
class ClassSelector : public CSSSelector {
  public:
    explicit ClassSelector(std::string_view class_name)
        : CSSSelector(SelectorType::Class),
          m_class_name(class_name) {}

    [[nodiscard]] bool matches(const Element& element) const override;

    [[nodiscard]] std::string to_string() const override {
        return "." + std::string(m_class_name);
    }

    [[nodiscard]] std::string_view class_name() const noexcept {
        return m_class_name;
    }

    [[nodiscard]] SelectorSpecificity calculate_specificity() const override {
        return {.inline_style = 0, .ids = 0, .classes = 1, .elements = 0};  // 类选择器优先级为10
    }

    [[nodiscard]] bool can_quick_reject(const Element& element) const override;

  private:
    std::string_view m_class_name;
};

// ID选择器 #id-name
class IdSelector : public CSSSelector {
  public:
    explicit IdSelector(std::string_view id_name)
        : CSSSelector(SelectorType::Id),
          m_id_name(id_name) {}

    [[nodiscard]] bool matches(const Element& element) const override;

    [[nodiscard]] std::string to_string() const override {
        return "#" + std::string(m_id_name);
    }

    [[nodiscard]] std::string_view id_name() const noexcept {
        return m_id_name;
    }

    [[nodiscard]] SelectorSpecificity calculate_specificity() const override {
        return {.inline_style = 0, .ids = 1, .classes = 0, .elements = 0};  // ID选择器优先级为100
    }

    [[nodiscard]] bool can_quick_reject(const Element& element) const override;

  private:
    std::string_view m_id_name;
};

// 属性选择器 [attr], [attr=value]
class AttributeSelector : public CSSSelector {
  public:
    AttributeSelector(std::string_view attr_name, AttributeOperator op, std::string_view value = "")
        : CSSSelector(SelectorType::Attribute),
          m_attr_name(attr_name),
          m_operator(op),
          m_value(value) {}

    [[nodiscard]] bool        matches(const Element& element) const override;
    [[nodiscard]] std::string to_string() const override;

    [[nodiscard]] std::string_view attr_name() const noexcept {
        return m_attr_name;
    }

    [[nodiscard]] AttributeOperator operator_type() const noexcept {
        return m_operator;
    }

    [[nodiscard]] std::string_view value() const noexcept {
        return m_value;
    }

    [[nodiscard]] SelectorSpecificity calculate_specificity() const override {
        return {.inline_style = 0, .ids = 0, .classes = 1, .elements = 0};  // 属性选择器优先级为10
    }

  private:
    std::string_view  m_attr_name;
    AttributeOperator m_operator;
    std::string_view  m_value;

    [[nodiscard]] bool matches_attribute_value(std::string_view attr_value) const;
};

// 组合选择器基类
class CombinatorSelector : public CSSSelector {
  public:
    CombinatorSelector(const SelectorType type, std::unique_ptr<CSSSelector> left, std::unique_ptr<CSSSelector> right)
        : CSSSelector(type),
          m_left(std::move(left)),
          m_right(std::move(right)) {}

    [[nodiscard]] const CSSSelector* left() const noexcept {
        return m_left.get();
    }

    [[nodiscard]] const CSSSelector* right() const noexcept {
        return m_right.get();
    }

    [[nodiscard]] SelectorSpecificity calculate_specificity() const override {
        const auto left_spec  = m_left ? m_left->calculate_specificity() : SelectorSpecificity{};
        const auto right_spec = m_right ? m_right->calculate_specificity() : SelectorSpecificity{};
        return {.inline_style = left_spec.inline_style + right_spec.inline_style, .ids = left_spec.ids + right_spec.ids, .classes = left_spec.classes + right_spec.classes, .elements = left_spec.elements + right_spec.elements};
    }

  protected:
    std::unique_ptr<CSSSelector> m_left;
    std::unique_ptr<CSSSelector> m_right;
};

// 后代选择器 div p
class DescendantSelector : public CombinatorSelector {
  public:
    DescendantSelector(std::unique_ptr<CSSSelector> left, std::unique_ptr<CSSSelector> right)
        : CombinatorSelector(SelectorType::Descendant, std::move(left), std::move(right)) {}

    [[nodiscard]] bool        matches(const Element& element) const override;
    [[nodiscard]] std::string to_string() const override;
};

// 子选择器 div > p
class ChildSelector : public CombinatorSelector {
  public:
    ChildSelector(std::unique_ptr<CSSSelector> left, std::unique_ptr<CSSSelector> right)
        : CombinatorSelector(SelectorType::Child, std::move(left), std::move(right)) {}

    [[nodiscard]] bool        matches(const Element& element) const override;
    [[nodiscard]] std::string to_string() const override;
};

// 相邻兄弟选择器 div + p
class AdjacentSiblingSelector : public CombinatorSelector {
  public:
    /**
     * @brief 构造函数
     * @param left 左侧选择器（前一个兄弟元素的选择器）
     * @param right 右侧选择器（当前元素的选择器）
     */
    AdjacentSiblingSelector(std::unique_ptr<CSSSelector> left, std::unique_ptr<CSSSelector> right)
        : CombinatorSelector(SelectorType::Adjacent, std::move(left), std::move(right)) {}

    /**
     * @brief 检查元素是否匹配相邻兄弟选择器
     * @param element 要检查的元素
     * @return 如果匹配返回true，否则返回false
     */
    [[nodiscard]] bool matches(const Element& element) const override;

    /**
     * @brief 将选择器转换为字符串表示
     * @return 选择器的字符串形式
     */
    [[nodiscard]] std::string to_string() const override;
};

// 通用兄弟选择器 div ~ p
class GeneralSiblingSelector : public CombinatorSelector {
  public:
    /**
     * @brief 构造函数
     * @param left 左侧选择器（前面兄弟元素的选择器）
     * @param right 右侧选择器（当前元素的选择器）
     */
    GeneralSiblingSelector(std::unique_ptr<CSSSelector> left, std::unique_ptr<CSSSelector> right)
        : CombinatorSelector(SelectorType::Sibling, std::move(left), std::move(right)) {}

    /**
     * @brief 检查元素是否匹配通用兄弟选择器
     * @param element 要检查的元素
     * @return 如果匹配返回true，否则返回false
     */
    [[nodiscard]] bool matches(const Element& element) const override;

    /**
     * @brief 将选择器转换为字符串表示
     * @return 选择器的字符串形式
     */
    [[nodiscard]] std::string to_string() const override;
};

// 复合选择器 - 用于组合多个简单选择器 (如 div.class#id)
class CompoundSelector : public CSSSelector {
  public:
    CompoundSelector()
        : CSSSelector(SelectorType::Compound) {}

    void                      add_selector(std::unique_ptr<CSSSelector> selector);
    [[nodiscard]] bool        matches(const Element& element) const override;
    [[nodiscard]] std::string to_string() const override;

    [[nodiscard]] SelectorSpecificity calculate_specificity() const override {
        SelectorSpecificity total{.inline_style = 0, .ids = 0, .classes = 0, .elements = 0};
        for (const auto& selector : m_selectors) {
            const auto [inline_style, ids, classes, elements] = selector->calculate_specificity();
            total.inline_style += inline_style;
            total.ids += ids;
            total.classes += classes;
            total.elements += elements;
        }
        return total;
    }

    [[nodiscard]] const std::vector<std::unique_ptr<CSSSelector>>& selectors() const noexcept {
        return m_selectors;
    }

    [[nodiscard]] bool empty() const noexcept {
        return m_selectors.empty();
    }

    [[nodiscard]] size_t size() const noexcept {
        return m_selectors.size();
    }

  private:
    std::vector<std::unique_ptr<CSSSelector>> m_selectors;
};

#include "hps/utils/string_pool.hpp"

// 选择器列表 - 用于逗号分隔的选择器组 (如 div, p, .class)
class SelectorList {
  public:
    void                      add_selector(std::unique_ptr<CSSSelector> selector);
    [[nodiscard]] bool        matches(const Element& element) const;
    [[nodiscard]] std::string to_string() const;

    // 获取最高优先级
    [[nodiscard]] SelectorSpecificity get_max_specificity() const {
        SelectorSpecificity max_spec{.inline_style = 0, .ids = 0, .classes = 0, .elements = 0};
        for (const auto& selector : m_selectors) {
            if (auto spec = selector->calculate_specificity(); max_spec < spec) {
                max_spec = spec;
            }
        }
        return max_spec;
    }

    [[nodiscard]] const std::vector<std::unique_ptr<CSSSelector>>& selectors() const noexcept {
        return m_selectors;
    }

    [[nodiscard]] bool empty() const noexcept {
        return m_selectors.empty();
    }

    [[nodiscard]] size_t size() const noexcept {
        return m_selectors.size();
    }

    void set_pool(std::shared_ptr<StringPool> pool) {
        m_pool = std::move(pool);
    }

  private:
    std::vector<std::unique_ptr<CSSSelector>> m_selectors;
    std::shared_ptr<StringPool>               m_pool;
};
}  // namespace hps