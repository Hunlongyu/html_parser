#include "hps/query/css/css_matcher.hpp"
#include "hps/query/css/css_parser.hpp"
#include "hps/query/css/css_utils.hpp"

#include "hps/core/element.hpp"
#include "hps/utils/exception.hpp"
#include "hps/utils/string_utils.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <regex>

namespace {

std::vector<std::string_view> split_selector_arguments(const std::string_view input) {
    std::vector<std::string_view> parts;
    size_t                        part_start   = 0;
    int                           paren_depth  = 0;
    int                           bracket_depth = 0;
    char                          quote_char   = '\0';

    for (size_t i = 0; i < input.size(); ++i) {
        const char current = input[i];

        if (quote_char != '\0') {
            if (current == quote_char) {
                quote_char = '\0';
            } else if (current == '\\' && i + 1 < input.size()) {
                ++i;
            }
            continue;
        }

        if (current == '"' || current == '\'') {
            quote_char = current;
            continue;
        }

        if (current == '(') {
            ++paren_depth;
            continue;
        }
        if (current == ')' && paren_depth > 0) {
            --paren_depth;
            continue;
        }
        if (current == '[') {
            ++bracket_depth;
            continue;
        }
        if (current == ']' && bracket_depth > 0) {
            --bracket_depth;
            continue;
        }

        if (current == ',' && paren_depth == 0 && bracket_depth == 0) {
            parts.push_back(input.substr(part_start, i - part_start));
            part_start = i + 1;
        }
    }

    parts.push_back(input.substr(part_start));
    return parts;
}

const hps::Element* first_element_child(const hps::Node* parent) {
    for (auto child = parent ? parent->first_child() : nullptr; child; child = child->next_sibling()) {
        if (child->is_element()) {
            return child->as_element();
        }
    }
    return nullptr;
}

const hps::Element* last_element_child(const hps::Node* parent) {
    for (auto child = parent ? parent->last_child() : nullptr; child; child = child->previous_sibling()) {
        if (child->is_element()) {
            return child->as_element();
        }
    }
    return nullptr;
}

bool matches_has_selector(const hps::Element& element, std::string_view selector) {
    selector = hps::trim_whitespace(selector);
    if (selector.empty()) {
        return false;
    }

    enum class RelativeMode : std::uint8_t { Descendant, Child, Adjacent, Sibling };

    RelativeMode mode = RelativeMode::Descendant;
    if (selector.front() == '>') {
        mode = RelativeMode::Child;
        selector.remove_prefix(1);
    } else if (selector.front() == '+') {
        mode = RelativeMode::Adjacent;
        selector.remove_prefix(1);
    } else if (selector.front() == '~') {
        mode = RelativeMode::Sibling;
        selector.remove_prefix(1);
    }

    selector = hps::trim_whitespace(selector);
    if (selector.empty()) {
        return false;
    }

    const auto selector_list = hps::parse_css_selector_cached(selector);
    if (!selector_list || selector_list->empty()) {
        return false;
    }

    switch (mode) {
        case RelativeMode::Descendant:
            return hps::CSSMatcher::find_first(element, *selector_list) != nullptr;
        case RelativeMode::Child:
            for (auto child = element.first_child(); child; child = child->next_sibling()) {
                if (const auto* child_element = child->as_element(); child_element && selector_list->matches(*child_element)) {
                    return true;
                }
            }
            return false;
        case RelativeMode::Adjacent:
            for (auto sibling = element.next_sibling(); sibling; sibling = sibling->next_sibling()) {
                if (const auto* sibling_element = sibling->as_element()) {
                    return selector_list->matches(*sibling_element);
                }
            }
            return false;
        case RelativeMode::Sibling:
            for (auto sibling = element.next_sibling(); sibling; sibling = sibling->next_sibling()) {
                if (const auto* sibling_element = sibling->as_element(); sibling_element && selector_list->matches(*sibling_element)) {
                    return true;
                }
            }
            return false;
    }

    return false;
}

bool matches_has_argument(const hps::Element& element, const std::string_view argument) {
    if (argument.empty()) {
        return false;
    }

    for (const auto part : split_selector_arguments(argument)) {
        if (matches_has_selector(element, part)) {
            return true;
        }
    }

    return false;
}

hps::SelectorSpecificity calculate_has_specificity(const std::string_view argument) {
    hps::SelectorSpecificity max_specificity{};

    for (auto part : split_selector_arguments(argument)) {
        part = hps::trim_whitespace(part);
        if (part.empty()) {
            continue;
        }

        if (part.front() == '>' || part.front() == '+' || part.front() == '~') {
            part.remove_prefix(1);
            part = hps::trim_whitespace(part);
        }

        if (part.empty()) {
            continue;
        }

        if (const auto selector_list = hps::parse_css_selector_cached(part); selector_list && !selector_list->empty()) {
            if (const auto specificity = selector_list->get_max_specificity(); max_specificity < specificity) {
                max_specificity = specificity;
            }
        }
    }

    return max_specificity;
}

}  // namespace

namespace hps {

// ==================== CSSParser Implementation ====================

CSSParser::CSSParser(const std::string_view selector, Options options)
    : m_pool(std::make_shared<StringPool>()),
      m_lexer(selector, *m_pool),
      m_options(std::move(options)) {}

bool CSSParser::validate() {
    m_options.error_handling = ErrorHandlingMode::Lenient;
    parse_selector_list();
    return !has_errors();
}

std::unique_ptr<SelectorList> CSSParser::parse_selector_list() {
    auto selector_list = std::make_unique<SelectorList>();
    selector_list->set_pool(m_pool);

    do {
        skip_whitespace();
        if (!has_more_tokens()) {
            break;
        }

        if (auto selector = parse_selector()) {
            selector_list->add_selector(std::move(selector));
        }

        skip_whitespace();
        if (match_token(CSSLexer::CSSTokenType::Comma)) {
            consume_token(CSSLexer::CSSTokenType::Comma);
        } else {
            break;
        }
    } while (has_more_tokens());

    return selector_list;
}

std::unique_ptr<CSSSelector> CSSParser::parse_selector() {
    std::unique_ptr<CSSSelector> left = parse_compound_selector();
    if (!left) {
        return nullptr;
    }

    while (has_more_tokens()) {
        if (match_token(CSSLexer::CSSTokenType::Comma) || match_token(CSSLexer::CSSTokenType::EndOfFile)) {
            break;
        }

        // 检查是否有空白字符
        bool has_whitespace = false;
        while (match_token(CSSLexer::CSSTokenType::Whitespace)) {
            m_lexer.next_token();  // 消费空白字符token
            has_whitespace = true;
        }

        // 检查显式组合器
        SelectorType combinator = SelectorType::Universal;
        const auto   token      = m_lexer.peek_token();

        if (token.type == CSSLexer::CSSTokenType::Greater) {
            m_lexer.next_token();
            combinator = SelectorType::Child;
        } else if (token.type == CSSLexer::CSSTokenType::Plus) {
            m_lexer.next_token();
            combinator = SelectorType::Adjacent;
        } else if (token.type == CSSLexer::CSSTokenType::Tilde) {
            m_lexer.next_token();
            combinator = SelectorType::Sibling;
        } else if (has_whitespace) {
            // 如果有空白字符但没有显式组合器，则为后代选择器
            combinator = SelectorType::Descendant;
        } else {
            // 没有空白字符也没有显式组合器，结束解析
            break;
        }

        // 跳过组合器后的空白字符
        while (match_token(CSSLexer::CSSTokenType::Whitespace)) {
            m_lexer.next_token();
        }

        auto right = parse_compound_selector();
        if (!right) {
            if (combinator != SelectorType::Descendant) {
                add_error("Expected selector after combinator");
            }
            break;
        }

        // 创建组合选择器
        switch (combinator) {
            case SelectorType::Descendant:
                left = std::make_unique<DescendantSelector>(std::move(left), std::move(right));
                break;
            case SelectorType::Child:
                left = std::make_unique<ChildSelector>(std::move(left), std::move(right));
                break;
            case SelectorType::Adjacent:
                left = std::make_unique<AdjacentSiblingSelector>(std::move(left), std::move(right));
                break;
            case SelectorType::Sibling:
                left = std::make_unique<GeneralSiblingSelector>(std::move(left), std::move(right));
                break;
            default:
                break;
        }
    }

    return left;
}

std::unique_ptr<CSSSelector> CSSParser::parse_compound_selector() {
    auto compound = std::make_unique<CompoundSelector>();

    while (has_more_tokens()) {
        const auto token = m_lexer.peek_token();
        if (token.type == CSSLexer::CSSTokenType::Greater || token.type == CSSLexer::CSSTokenType::Plus || token.type == CSSLexer::CSSTokenType::Tilde || token.type == CSSLexer::CSSTokenType::Comma || token.type == CSSLexer::CSSTokenType::EndOfFile) {
            break;
        }

        // 检查是否是空白字符（后代选择器）
        if (token.type == CSSLexer::CSSTokenType::Whitespace) {
            break;
        }

        if (auto simple_selector = parse_simple_selector()) {
            compound->add_selector(std::move(simple_selector));
        } else {
            break;
        }
    }

    if (compound->empty()) {
        return nullptr;
    }

    // 如果只有一个选择器，直接返回该选择器
    if (compound->selectors().size() == 1) {
        auto& selectors = const_cast<std::vector<std::unique_ptr<CSSSelector>>&>(compound->selectors());
        auto  result    = std::move(selectors[0]);
        return result;
    }

    return std::move(compound);
}

std::unique_ptr<CSSSelector> CSSParser::parse_simple_selector() {
    const auto token = m_lexer.peek_token();

    switch (token.type) {
        case CSSLexer::CSSTokenType::Identifier:
            return parse_type_selector();

        case CSSLexer::CSSTokenType::Dot:
            return parse_class_selector();

        case CSSLexer::CSSTokenType::Hash:
            return parse_id_selector();

        case CSSLexer::CSSTokenType::LeftBracket:
            return parse_attribute_selector();

        case CSSLexer::CSSTokenType::Star:
            m_lexer.next_token();
            return std::make_unique<UniversalSelector>();

        case CSSLexer::CSSTokenType::Colon:
        case CSSLexer::CSSTokenType::DoubleColon:
            return parse_pseudo_selector();

        default:
            return nullptr;
    }
}

std::unique_ptr<TypeSelector> CSSParser::parse_type_selector() {
    auto token = m_lexer.next_token();
    if (token.type != CSSLexer::CSSTokenType::Identifier) {
        add_error("Expected identifier for type selector");
        return nullptr;
    }
    
    // Normalize to lowercase and store in pool
    std::string tag(token.value);
    std::ranges::transform(tag, tag.begin(), [](unsigned char c){ return std::tolower(c); });
    
    return std::make_unique<TypeSelector>(m_pool->add(tag));
}

std::unique_ptr<ClassSelector> CSSParser::parse_class_selector() {
    consume_token(CSSLexer::CSSTokenType::Dot);

    auto token = m_lexer.next_token();
    if (token.type != CSSLexer::CSSTokenType::Identifier) {
        add_error("Expected identifier after '.' in class selector");
        return nullptr;
    }

    // Class names are case-sensitive in CSS, but check HTML spec?
    // Generally treated as case-sensitive.
    return std::make_unique<ClassSelector>(token.value);
}

std::unique_ptr<IdSelector> CSSParser::parse_id_selector() {
    auto token = m_lexer.next_token();
    if (token.type != CSSLexer::CSSTokenType::Hash) {
        add_error("Expected hash token for ID selector");
        return nullptr;
    }

    return std::make_unique<IdSelector>(token.value);
}

std::unique_ptr<AttributeSelector> CSSParser::parse_attribute_selector() {
    consume_token(CSSLexer::CSSTokenType::LeftBracket);

    const auto attr_token = m_lexer.next_token();
    if (attr_token.type != CSSLexer::CSSTokenType::Identifier) {
        add_error("Expected attribute name in attribute selector");
        return nullptr;
    }

    // Normalize attribute name to lowercase
    std::string attr_name(attr_token.value);
    std::ranges::transform(attr_name, attr_name.begin(), [](unsigned char c){ return std::tolower(c); });
    std::string_view attr_name_view = m_pool->add(attr_name);

    AttributeOperator op = AttributeOperator::Exists;
    std::string_view value;

    // 检查是否有操作符
    const auto next_token = m_lexer.peek_token();
    if (next_token.type != CSSLexer::CSSTokenType::RightBracket) {
        op = parse_attribute_operator();

        const auto value_token = m_lexer.next_token();
        if (value_token.type == CSSLexer::CSSTokenType::String || value_token.type == CSSLexer::CSSTokenType::Identifier) {
            value = value_token.value;
        } else {
            add_error("Expected value after attribute operator");
            return nullptr;
        }
    }

    consume_token(CSSLexer::CSSTokenType::RightBracket);

    return std::make_unique<AttributeSelector>(attr_name_view, op, value);
}

std::unique_ptr<CSSSelector> CSSParser::parse_pseudo_selector() {
    const auto token = m_lexer.peek_token();

    if (token.type == CSSLexer::CSSTokenType::DoubleColon) {
        return parse_pseudo_element();
    }
    if (token.type == CSSLexer::CSSTokenType::Colon) {
        return parse_pseudo_class();
    }

    return nullptr;
}

std::unique_ptr<CSSSelector> CSSParser::parse_pseudo_class() {
    consume_token(CSSLexer::CSSTokenType::Colon);

    const auto name_token = m_lexer.next_token();
    if (name_token.type != CSSLexer::CSSTokenType::Identifier) {
        add_error("Expected pseudo-class name after ':'");
        return nullptr;
    }

    const std::string_view name = name_token.value;
    std::string argument_str; // Temp string to build argument

    // 检查是否有参数
    if (match_token(CSSLexer::CSSTokenType::LeftParen)) {
        consume_token(CSSLexer::CSSTokenType::LeftParen);

        // 改进的参数解析
        int paren_depth = 0;

        while (has_more_tokens()) {
            const auto token = m_lexer.peek_token();

            if (token.type == CSSLexer::CSSTokenType::LeftParen) {
                paren_depth++;
                m_lexer.next_token();
                argument_str += m_lexer.source_span(token);
            } else if (token.type == CSSLexer::CSSTokenType::RightParen) {
                if (paren_depth == 0) {
                    // 这是结束的右括号
                    break;
                }
                paren_depth--;
                m_lexer.next_token();
                argument_str += m_lexer.source_span(token);
            } else if (token.type == CSSLexer::CSSTokenType::EndOfFile) {
                add_error("Unexpected end of input in pseudo-class arguments");
                return nullptr;
            } else {
                // 消费其他类型的 token
                m_lexer.next_token();
                if (token.type != CSSLexer::CSSTokenType::Whitespace || !argument_str.empty()) {
                    argument_str += m_lexer.source_span(token);
                }
            }
        }
        consume_token(CSSLexer::CSSTokenType::RightParen);
    }

    // 映射伪类名称到类型
    PseudoClassSelector::PseudoType type;
    if (name == "first-child") {
        type = PseudoClassSelector::PseudoType::FirstChild;
    } else if (name == "last-child") {
        type = PseudoClassSelector::PseudoType::LastChild;
    } else if (name == "nth-child") {
        type = PseudoClassSelector::PseudoType::NthChild;
    } else if (name == "nth-last-child") {
        type = PseudoClassSelector::PseudoType::NthLastChild;
    } else if (name == "first-of-type") {
        type = PseudoClassSelector::PseudoType::FirstOfType;
    } else if (name == "last-of-type") {
        type = PseudoClassSelector::PseudoType::LastOfType;
    } else if (name == "nth-of-type") {
        type = PseudoClassSelector::PseudoType::NthOfType;
    } else if (name == "nth-last-of-type") {
        type = PseudoClassSelector::PseudoType::NthLastOfType;
    } else if (name == "only-child") {
        type = PseudoClassSelector::PseudoType::OnlyChild;
    } else if (name == "only-of-type") {
        type = PseudoClassSelector::PseudoType::OnlyOfType;
    } else if (name == "not") {
        type = PseudoClassSelector::PseudoType::Not;
    } else if (name == "is") {
        type = PseudoClassSelector::PseudoType::Is;
    } else if (name == "where") {
        type = PseudoClassSelector::PseudoType::Where;
    } else if (name == "has") {
        type = PseudoClassSelector::PseudoType::Has;
    } else if (name == "empty") {
        type = PseudoClassSelector::PseudoType::Empty;
    } else if (name == "root") {
        type = PseudoClassSelector::PseudoType::Root;
    } else if (name == "hover") {
        type = PseudoClassSelector::PseudoType::Hover;
    } else if (name == "active") {
        type = PseudoClassSelector::PseudoType::Active;
    } else if (name == "focus") {
        type = PseudoClassSelector::PseudoType::Focus;
    } else if (name == "disabled") {
        type = PseudoClassSelector::PseudoType::Disabled;
    } else if (name == "enabled") {
        type = PseudoClassSelector::PseudoType::Enabled;
    } else if (name == "checked") {
        type = PseudoClassSelector::PseudoType::Checked;
    } else if (name == "visited") {
        type = PseudoClassSelector::PseudoType::Visited;
    } else if (name == "link") {
        type = PseudoClassSelector::PseudoType::Link;
    } else {
        add_error("Unsupported pseudo-class: " + std::string(name));
        return nullptr;
    }

    std::string_view argument_view;
    if (!argument_str.empty()) {
        argument_view = m_pool->add(argument_str);
    }

    std::unique_ptr<SelectorList> sub_selectors = nullptr;

    // 对于需要子选择器列表的伪类，解析参数
    if (type == PseudoClassSelector::PseudoType::Not || type == PseudoClassSelector::PseudoType::Is || type == PseudoClassSelector::PseudoType::Where) {
        if (!argument_str.empty()) {
            try {
                CSSParser inner_parser(argument_view);
                sub_selectors = inner_parser.parse_selector_list();
            } catch (const HPSException& e) {
                 add_error("Invalid selector in pseudo-class argument: " + std::string(e.what()));
                 return nullptr;
            }
        }
    }

    return std::make_unique<PseudoClassSelector>(type, argument_view, std::move(sub_selectors));
}

std::unique_ptr<CSSSelector> CSSParser::parse_pseudo_element() {
    consume_token(CSSLexer::CSSTokenType::DoubleColon);

    const auto name_token = m_lexer.next_token();
    if (name_token.type != CSSLexer::CSSTokenType::Identifier) {
        add_error("Expected pseudo-element name after '::'");
        return nullptr;
    }

    const std::string_view name = name_token.value;

    // 映射伪元素名称到类型
    PseudoElementSelector::ElementType type;
    if (name == "before") {
        type = PseudoElementSelector::ElementType::Before;
    } else if (name == "after") {
        type = PseudoElementSelector::ElementType::After;
    } else if (name == "first-line") {
        type = PseudoElementSelector::ElementType::FirstLine;
    } else if (name == "first-letter") {
        type = PseudoElementSelector::ElementType::FirstLetter;
    } else {
        add_error("Unsupported pseudo-element: " + std::string(name));
        return nullptr;
    }

    return std::make_unique<PseudoElementSelector>(type);
}

SelectorType CSSParser::parse_combinator() {
    const auto token = m_lexer.peek_token();

    // 检查空白字符（后代选择器）
    if (token.type == CSSLexer::CSSTokenType::Whitespace) {
        m_lexer.next_token();  // 消费空白字符token
        return SelectorType::Descendant;
    }

    if (token.type == CSSLexer::CSSTokenType::Greater) {
        m_lexer.next_token();
        return SelectorType::Child;
    }
    if (token.type == CSSLexer::CSSTokenType::Plus) {
        m_lexer.next_token();
        return SelectorType::Adjacent;
    }
    if (token.type == CSSLexer::CSSTokenType::Tilde) {
        m_lexer.next_token();
        return SelectorType::Sibling;
    }
    return SelectorType::Universal;
}

AttributeOperator CSSParser::parse_attribute_operator() {
    if (match_token(CSSLexer::CSSTokenType::Equals)) {
        m_lexer.next_token();
        return AttributeOperator::Equals;
    }
    if (match_token(CSSLexer::CSSTokenType::Contains)) {
        m_lexer.next_token();
        return AttributeOperator::Contains;
    }
    if (match_token(CSSLexer::CSSTokenType::StartsWith)) {
        m_lexer.next_token();
        return AttributeOperator::StartsWith;
    }
    if (match_token(CSSLexer::CSSTokenType::EndsWith)) {
        m_lexer.next_token();
        return AttributeOperator::EndsWith;
    }
    if (match_token(CSSLexer::CSSTokenType::WordMatch)) {
        m_lexer.next_token();
        return AttributeOperator::WordMatch;
    }
    if (match_token(CSSLexer::CSSTokenType::LangMatch)) {
        m_lexer.next_token();
        return AttributeOperator::LangMatch;
    }

    return AttributeOperator::Exists;
}

void CSSParser::consume_token(const CSSLexer::CSSTokenType expected) {
    const auto token = m_lexer.next_token();
    if (token.type != expected) {
        add_error("Unexpected token", token);
    }
}

bool CSSParser::match_token(const CSSLexer::CSSTokenType type) {
    return m_lexer.peek_token().type == type;
}

void CSSParser::skip_whitespace() {
    while (has_more_tokens() && match_token(CSSLexer::CSSTokenType::Whitespace)) {
        m_lexer.next_token();
    }
}

bool CSSParser::has_more_tokens() {
    return m_lexer.has_more_tokens() && !match_token(CSSLexer::CSSTokenType::EndOfFile);
}

void CSSParser::add_error(const std::string& message) {
    m_errors.emplace_back(ErrorCode::InvalidSelector, message, m_lexer.current_position());

    if (m_options.error_handling == ErrorHandlingMode::Strict) {
        throw_parse_error(message);
    }
}

void CSSParser::add_error(const std::string& message, const CSSLexer::CSSToken& token) {
    m_errors.emplace_back(ErrorCode::InvalidSelector, message, token.position);

    if (m_options.error_handling == ErrorHandlingMode::Strict) {
        throw_parse_error(message);
    }
}

bool CSSParser::try_recover_from_error() {
    // 简单的错误恢复：跳到下一个逗号或结束
    while (has_more_tokens()) {
        const auto token = m_lexer.next_token();
        if (token.type == CSSLexer::CSSTokenType::Comma || token.type == CSSLexer::CSSTokenType::EndOfFile) {
            return true;
        }
    }
    return false;
}

void CSSParser::throw_parse_error(const std::string& message) const {
    throw HPSException(ErrorCode::InvalidSelector, "CSS Parser Error: " + message, Location(m_lexer.current_position(), m_lexer.current_line(), m_lexer.current_column()));
}

// ==================== PseudoClassSelector Implementation ====================

bool PseudoClassSelector::matches(const Element& element) const {
    switch (m_pseudo_type) {
        case PseudoType::FirstChild: {
            // :first-child - 检查是否为父元素的第一个子元素
            auto parent = element.parent();
            if (!parent) {
                return false;
            }
            return first_element_child(parent) == &element;
        }

        case PseudoType::LastChild: {
            // :last-child - 检查是否为父元素的最后一个子元素
            auto parent = element.parent();
            if (!parent) {
                return false;
            }
            return last_element_child(parent) == &element;
        }

        case PseudoType::NthChild: {
            // :nth-child(n) - 检查是否为第n个子元素
            auto parent = element.parent();
            if (!parent) {
                return false;
            }

            int index = 1;
            for (auto child = parent->first_child(); child; child = child->next_sibling()) {
                if (child->is_element()) {
                    if (child == &element) {
                        return matches_nth_expression(m_argument, index);
                    }
                    ++index;
                }
            }
            return false;
        }

        case PseudoType::NthLastChild: {
            // :nth-last-child(n) - 检查是否为倒数第n个子元素
            auto parent = element.parent();
            if (!parent) {
                return false;
            }

            int reverse_index = 1;
            for (auto child = parent->last_child(); child; child = child->previous_sibling()) {
                if (child->is_element()) {
                    if (child == &element) {
                        return matches_nth_expression(m_argument, reverse_index);
                    }
                    ++reverse_index;
                }
            }
            return false;
        }

        case PseudoType::NthOfType: {
            // :nth-of-type(n) - 检查是否为同类型中的第n个元素
            int index = get_type_index(element, false);
            return matches_nth_expression(m_argument, index);
        }

        case PseudoType::NthLastOfType: {
            // :nth-last-of-type(n) - 检查是否为同类型中的倒数第n个元素
            int index = get_type_index(element, true);
            return matches_nth_expression(m_argument, index);
        }

        case PseudoType::FirstOfType: {
            // :first-of-type - 检查是否为同标签名的第一个元素
            return get_type_index(element) == 1;
        }

        case PseudoType::LastOfType: {
            // :last-of-type - 检查是否为同标签名的最后一个元素
            return get_type_index(element, true) == 1;
        }

        case PseudoType::OnlyChild: {
            // :only-child - 检查是否为唯一的子元素
            auto parent = element.parent();
            if (!parent) {
                return false;
            }

            int element_count = 0;
            for (auto child = parent->first_child(); child; child = child->next_sibling()) {
                if (child->is_element()) {
                    ++element_count;
                    if (element_count > 1) {
                        return false;
                    }
                }
            }
            return element_count == 1;
        }

        case PseudoType::OnlyOfType: {
            // :only-of-type - 检查是否为同标签名的唯一元素
            return count_siblings_of_type(element) == 1;
        }

        case PseudoType::Empty: {
            // :empty - 检查元素是否为空（无子元素和文本内容）
            for (auto child = element.first_child(); child; child = child->next_sibling()) {
                if (child->type() == NodeType::Element) {
                    return false;
                }
                if (child->type() == NodeType::Text) {
                    auto text_content = child->text_content();
                    // 检查是否只包含空白字符
                    if (!text_content.empty() && !std::ranges::all_of(text_content, is_whitespace)) {
                        return false;
                    }
                }
            }
            return true;
        }

        case PseudoType::Root: {
            // :root - 检查是否为根元素
            auto parent = element.parent();
            return parent == nullptr || parent->type() == NodeType::Document;
        }

        case PseudoType::Not: {
            if (!m_sub_selectors) {
                return false; // :not() 必须有参数
            }
            // :not(selector-list) 只要有一个匹配就返回false
            return !m_sub_selectors->matches(element);
        }

        case PseudoType::Is:
        case PseudoType::Where: {
            if (!m_sub_selectors) {
                return false;
            }
            // :is/:where 只要有一个匹配就返回true
            return m_sub_selectors->matches(element);
        }

        case PseudoType::Has: {
             if (m_argument.empty()) {
                return false;
            }

            return matches_has_argument(element, m_argument);
        }

        // 状态伪类通常需要外部状态信息，这里提供基础实现
        case PseudoType::Hover:
        case PseudoType::Active:
        case PseudoType::Focus:
            // 这些状态需要外部状态管理器支持
            return false;

        case PseudoType::Visited:
        case PseudoType::Link: {
            // 链接相关伪类，检查是否为a标签且有href属性
            if (!equals_ignore_case(element.tag_name(), "a")) {
                return false;
            }
            bool has_href = element.has_attribute("href");
            return (m_pseudo_type == PseudoType::Link) ? has_href : false;
        }

        case PseudoType::Disabled: {
            // :disabled - 检查表单元素是否被禁用
            return element.has_attribute("disabled");
        }

        case PseudoType::Enabled: {
            // :enabled - 检查表单元素是否启用
            static constexpr std::array<std::string_view, 7> form_elements = {"input", "button", "select", "textarea", "option", "optgroup", "fieldset"};
            const auto&                                 tag             = element.tag_name();
            const bool                                  is_form_element = std::ranges::any_of(form_elements, [&tag](const auto form_element) { return equals_ignore_case(tag, form_element); });
            return is_form_element && !element.has_attribute("disabled");
        }

        case PseudoType::Checked: {
            // :checked - 检查表单元素是否被选中
            if (const auto& tag = element.tag_name(); equals_ignore_case(tag, "input")) {
                if (const auto& type = element.get_attribute("type"); equals_ignore_case(type, "checkbox") || equals_ignore_case(type, "radio")) {
                    return element.has_attribute("checked");
                }
            } else if (equals_ignore_case(tag, "option")) {
                return element.has_attribute("selected");
            }
            return false;
        }
    }
    return false;
}

std::string PseudoClassSelector::to_string() const {
    switch (m_pseudo_type) {
        case PseudoType::FirstChild:
            return ":first-child";
        case PseudoType::LastChild:
            return ":last-child";
        case PseudoType::NthChild:
            return ":nth-child(" + std::string(m_argument) + ")";
        case PseudoType::NthLastChild:
            return ":nth-last-child(" + std::string(m_argument) + ")";
        case PseudoType::NthOfType:
            return ":nth-of-type(" + std::string(m_argument) + ")";
        case PseudoType::NthLastOfType:
            return ":nth-last-of-type(" + std::string(m_argument) + ")";
        case PseudoType::FirstOfType:
            return ":first-of-type";
        case PseudoType::LastOfType:
            return ":last-of-type";
        case PseudoType::OnlyChild:
            return ":only-child";
        case PseudoType::OnlyOfType:
            return ":only-of-type";
        case PseudoType::Empty:
            return ":empty";
        case PseudoType::Root:
            return ":root";
        case PseudoType::Not:
            return ":not(" + std::string(m_argument) + ")";
        case PseudoType::Is:
            return ":is(" + std::string(m_argument) + ")";
        case PseudoType::Where:
            return ":where(" + std::string(m_argument) + ")";
        case PseudoType::Has:
            return ":has(" + std::string(m_argument) + ")";
        case PseudoType::Hover:
            return ":hover";
        case PseudoType::Active:
            return ":active";
        case PseudoType::Focus:
            return ":focus";
        case PseudoType::Visited:
            return ":visited";
        case PseudoType::Link:
            return ":link";
        case PseudoType::Disabled:
            return ":disabled";
        case PseudoType::Enabled:
            return ":enabled";
        case PseudoType::Checked:
            return ":checked";
    }
    return ":unknown";
}

SelectorSpecificity PseudoClassSelector::calculate_specificity() const {
    if (m_pseudo_type == PseudoType::Where) {
        return SelectorSpecificity{}; // :where() 优先级总是0
    }

    if (m_pseudo_type == PseudoType::Has) {
        return calculate_has_specificity(m_argument);
    }

    if (m_pseudo_type == PseudoType::Is || m_pseudo_type == PseudoType::Not) {
        // :is(), :not(), :has() 优先级是其参数列表中优先级最高的选择器的优先级
        if (m_sub_selectors) {
            return m_sub_selectors->get_max_specificity();
        }
        return SelectorSpecificity{};
    }

    SelectorSpecificity spec{};
    spec.classes = 1;  // 其他伪类选择器增加类选择器计数
    return spec;
}

bool PseudoClassSelector::matches_nth_expression(std::string_view expression, const int index) {
    if (expression.empty()) {
        return false;
    }

    // 处理特殊关键字
    if (expression == "odd") {
        return index % 2 == 1;
    }
    if (expression == "even") {
        return index % 2 == 0;
    }
    
    std::string expr_str(expression);

    // 处理纯数字
    if (std::ranges::all_of(expr_str, is_digit)) {
        return index == std::stoi(expr_str);
    }

    // 处理an+b格式
    // 简化的解析器，支持如"2n+1"、"3n"、"-n+6"等格式
    const std::regex nth_regex(R"(^\s*([+-]?\d*)n\s*([+-]\s*\d+)?\s*$)");
    std::smatch      match;

    if (std::regex_match(expr_str, match, nth_regex)) {
        int a = 1;  // 默认系数
        int b = 0;  // 默认偏移

        // 解析系数a
        const std::string a_str = match[1].str();
        if (!a_str.empty()) {
            if (a_str == "+" || a_str.empty()) {
                a = 1;
            } else if (a_str == "-") {
                a = -1;
            } else {
                a = std::stoi(a_str);
            }
        }

        // 解析偏移b
        std::string b_str = match[2].str();
        if (!b_str.empty()) {
            // 移除空格
            b_str.erase(std::ranges::remove_if(b_str, is_whitespace).begin(), b_str.end());
            b = std::stoi(b_str);
        }

        // 检查是否匹配an+b公式
        if (a == 0) {
            return index == b;
        }

        // 对于正系数，检查(index - b) % a == 0 且 index >= b
        // 对于负系数，需要特殊处理
        if (a > 0) {
            return index >= b && (index - b) % a == 0;
        }
        return index <= b && (b - index) % (-a) == 0;
    }

    return false;
}

int PseudoClassSelector::count_siblings_of_type(const Element& element) {
    const auto parent = element.parent();
    if (!parent) {
        return 1;
    }

    int         count    = 0;
    const auto& tag_name = element.tag_name();

    for (auto child = parent->first_child(); child; child = child->next_sibling()) {
        if (child->type() == NodeType::Element) {
            const auto child_element = child->as_element();
            if (child_element && equals_ignore_case(child_element->tag_name(), tag_name)) {
                ++count;
            }
        }
    }

    return count;
}

int PseudoClassSelector::get_type_index(const Element& element, const bool from_end) {
    const auto parent = element.parent();
    if (!parent) {
        return 1;
    }

    const auto& tag_name = element.tag_name();

    if (from_end) {
        int index = 1;
        for (auto child = parent->last_child(); child; child = child->previous_sibling()) {
            if (child->type() == NodeType::Element) {
                const auto* child_element = child->as_element();
                if (child_element && equals_ignore_case(child_element->tag_name(), tag_name)) {
                    if (child_element == &element) {
                        return index;
                    }
                    ++index;
                }
            }
        }
        return 0;
    }

    int index = 1;
    for (auto child = parent->first_child(); child; child = child->next_sibling()) {
        if (child->type() == NodeType::Element) {
            const auto* child_element = child->as_element();
            if (child_element && equals_ignore_case(child_element->tag_name(), tag_name)) {
                if (child_element == &element) {
                    return index;
                }
                ++index;
            }
        }
    }

    return 0;
}

std::string PseudoElementSelector::to_string() const {
    switch (m_element_type) {
        case ElementType::Before:
            return "::before";
        case ElementType::After:
            return "::after";
        case ElementType::FirstLine:
            return "::first-line";
        case ElementType::FirstLetter:
            return "::first-letter";
    }
    return "::unknown";
}

/**
 * @brief 伪元素选择器匹配实现
 *
 * 伪元素选择器通常不直接匹配DOM元素，而是用于样式应用。
 * 在实际的CSS引擎中，伪元素会创建虚拟元素来应用样式。
 * 这里我们提供一个基础实现，总是返回false，因为DOM中不存在真实的伪元素。
 */
bool PseudoElementSelector::matches(const Element& element) const {
    // 伪元素选择器不直接匹配DOM元素
    // 在真实的CSS引擎中，伪元素用于创建虚拟元素来应用样式
    // 例如 ::before 和 ::after 会在元素内容前后插入虚拟内容
    // ::first-line 和 ::first-letter 会对文本的特定部分应用样式

    // 对于查询目的，我们可以检查元素是否支持该伪元素
    switch (m_element_type) {
        case ElementType::Before:
        case ElementType::After:
            // ::before 和 ::after 可以应用于大多数元素
            // 但通常不应用于替换元素（如 img, input 等）
            {
                const std::string& tag = element.tag_name();
                // 排除不支持 ::before/::after 的替换元素
                static constexpr std::array<std::string_view, 17> replaced_elements = {"img", "input", "textarea", "select", "option", "br", "hr", "area", "base", "col", "embed", "link", "meta", "param", "source", "track", "wbr"};
                return !std::ranges::any_of(replaced_elements, [&tag](const auto replaced_tag) { return equals_ignore_case(tag, replaced_tag); });
            }

        case ElementType::FirstLine:
        case ElementType::FirstLetter:
            // ::first-line 和 ::first-letter 只能应用于块级元素
            // 这里简化处理，检查是否为常见的块级元素
            {
                const std::string&                        tag            = element.tag_name();
                static constexpr std::array<std::string_view, 18> block_elements = {"div", "p", "h1", "h2", "h3", "h4", "h5", "h6", "article", "section", "header", "footer", "main", "aside", "nav", "blockquote", "pre", "address"};
                return std::ranges::any_of(block_elements, [&tag](const auto block_tag) { return equals_ignore_case(tag, block_tag); });
            }
    }
    return false;
}
}  // namespace hps
