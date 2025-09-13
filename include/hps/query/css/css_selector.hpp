#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace hps {

class Element;

// CSS选择器类型枚举
enum class SelectorType {
    Universal,   // *
    Type,        // div, p, span
    Class,       // .class-name
    Id,          // #id-name
    Attribute,   // [attr], [attr=value]
    Descendant,  // div p (空格)
    Child,       // div > p
    Adjacent,    // div + p
    Sibling      // div ~ p
};

// 属性选择器操作符
enum class AttributeOperator {
    Exists,      // [attr]
    Equals,      // [attr=value]
    Contains,    // [attr*=value]
    StartsWith,  // [attr^=value]
    EndsWith,    // [attr$=value]
    WordMatch,   // [attr~=value]
    LangMatch    // [attr|=value]
};

// CSS选择器AST节点基类
class CSSSelector {
  public:
    explicit CSSSelector(SelectorType type) : m_type(type) {}
    virtual ~CSSSelector() = default;

    SelectorType type() const {
        return m_type;
    }
    virtual bool        matches(const Element& element) const = 0;
    virtual std::string to_string() const                     = 0;

  protected:
    SelectorType m_type;
};

// 通用选择器 *
class UniversalSelector : public CSSSelector {
  public:
    UniversalSelector() : CSSSelector(SelectorType::Universal) {}
    bool        matches(const Element& element) const override;
    std::string to_string() const override {
        return "*";
    }
};

// 类型选择器 div, p, span
class TypeSelector : public CSSSelector {
  public:
    explicit TypeSelector(std::string tag_name) : CSSSelector(SelectorType::Type), m_tag_name(std::move(tag_name)) {}

    bool        matches(const Element& element) const override;
    std::string to_string() const override {
        return m_tag_name;
    }
    const std::string& tag_name() const {
        return m_tag_name;
    }

  private:
    std::string m_tag_name;
};

// 类选择器 .class-name
class ClassSelector : public CSSSelector {
  public:
    explicit ClassSelector(std::string class_name) : CSSSelector(SelectorType::Class), m_class_name(std::move(class_name)) {}

    bool        matches(const Element& element) const override;
    std::string to_string() const override {
        return "." + m_class_name;
    }
    const std::string& class_name() const {
        return m_class_name;
    }

  private:
    std::string m_class_name;
};

// ID选择器 #id-name
class IdSelector : public CSSSelector {
  public:
    explicit IdSelector(std::string id_name) : CSSSelector(SelectorType::Id), m_id_name(std::move(id_name)) {}

    bool        matches(const Element& element) const override;
    std::string to_string() const override {
        return "#" + m_id_name;
    }
    const std::string& id_name() const {
        return m_id_name;
    }

  private:
    std::string m_id_name;
};

// 属性选择器 [attr], [attr=value]
class AttributeSelector : public CSSSelector {
  public:
    AttributeSelector(std::string attr_name, AttributeOperator op, std::string value = "") : CSSSelector(SelectorType::Attribute), m_attr_name(std::move(attr_name)), m_operator(op), m_value(std::move(value)) {}

    bool        matches(const Element& element) const override;
    std::string to_string() const override;

    const std::string& attr_name() const {
        return m_attr_name;
    }
    AttributeOperator operator_type() const {
        return m_operator;
    }
    const std::string& value() const {
        return m_value;
    }

  private:
    std::string       m_attr_name;
    AttributeOperator m_operator;
    std::string       m_value;
};

// 组合选择器基类
class CombinatorSelector : public CSSSelector {
  public:
    CombinatorSelector(SelectorType type, std::unique_ptr<CSSSelector> left, std::unique_ptr<CSSSelector> right) : CSSSelector(type), m_left(std::move(left)), m_right(std::move(right)) {}

    const CSSSelector* left() const {
        return m_left.get();
    }
    const CSSSelector* right() const {
        return m_right.get();
    }

  protected:
    std::unique_ptr<CSSSelector> m_left;
    std::unique_ptr<CSSSelector> m_right;
};

// 后代选择器 div p
class DescendantSelector : public CombinatorSelector {
  public:
    DescendantSelector(std::unique_ptr<CSSSelector> left, std::unique_ptr<CSSSelector> right) : CombinatorSelector(SelectorType::Descendant, std::move(left), std::move(right)) {}

    bool        matches(const Element& element) const override;
    std::string to_string() const override;
};

// 子选择器 div > p
class ChildSelector : public CombinatorSelector {
  public:
    ChildSelector(std::unique_ptr<CSSSelector> left, std::unique_ptr<CSSSelector> right) : CombinatorSelector(SelectorType::Child, std::move(left), std::move(right)) {}

    bool        matches(const Element& element) const override;
    std::string to_string() const override;
};

// 复合选择器 - 用于组合多个简单选择器 (如 div.class#id)
class CompoundSelector : public CSSSelector {
  public:
    CompoundSelector() : CSSSelector(SelectorType::Type) {}  // 使用Type作为默认类型

    void        add_selector(std::unique_ptr<CSSSelector> selector);
    bool        matches(const Element& element) const override;
    std::string to_string() const override;

    const std::vector<std::unique_ptr<CSSSelector>>& selectors() const {
        return m_selectors;
    }
    bool empty() const {
        return m_selectors.empty();
    }

  private:
    std::vector<std::unique_ptr<CSSSelector>> m_selectors;
};

// 选择器列表 - 用于逗号分隔的选择器组 (如 div, p, .class)
class SelectorList {
  public:
    void        add_selector(std::unique_ptr<CSSSelector> selector);
    bool        matches(const Element& element) const;
    std::string to_string() const;

    const std::vector<std::unique_ptr<CSSSelector>>& selectors() const {
        return m_selectors;
    }
    bool empty() const {
        return m_selectors.empty();
    }
    size_t size() const {
        return m_selectors.size();
    }

  private:
    std::vector<std::unique_ptr<CSSSelector>> m_selectors;
};

}  // namespace hps