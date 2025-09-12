#include "hps/parsing/tree_builder.hpp"

#include "hps/core/document.hpp"
#include "hps/core/text_node.hpp"
#include "hps/utils/string_utils.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>

namespace hps {

TreeBuilder::TreeBuilder(Document* document) : m_document(document) {
    assert(m_document != nullptr);
    m_element_stack.reserve(32);  // 预分配空间提高性能
}

bool TreeBuilder::process_token(const Token& token) {
    try {
        switch (token.type()) {
            case TokenType::OPEN:
            case TokenType::CLOSE_SELF:
                process_start_tag(token);
                break;
            case TokenType::CLOSE:
                process_end_tag(token);
                break;
            case TokenType::TEXT:
            case TokenType::COMMENT:
                process_text(token);
                break;
            case TokenType::DONE:
                break;
            case TokenType::FORCE_QUIRKS:
                parse_error(ErrorCode::InvalidNesting, "Force quirks mode detected");
                break;
            case TokenType::DOCTYPE:
                break;
        }
        return true;
    } catch (const std::exception& e) {
        parse_error(ErrorCode::UnknownError, e.what());
        return false;
    }
}

bool TreeBuilder::finish() {
    // 关闭所有未闭合的元素
    while (!m_element_stack.empty()) {
        Element* element = m_element_stack.back();
        m_element_stack.pop_back();

        // 记录未闭合标签的警告
        parse_error(ErrorCode::UnclosedTag, "Unclosed tag: " + std::string(element->tag_name()));
    }

    return true;
}

Document* TreeBuilder::document() const noexcept {
    return m_document;
}

const std::vector<ParseError>& TreeBuilder::errors() const noexcept {
    return m_errors;
}

void TreeBuilder::process_start_tag(const Token& token) {
    auto     element = create_element(token);
    Element* raw_ptr = element.get();  // 保存原始指针用于栈管理

    // 插入到DOM树中
    insert_element(std::move(element));

    // 如果不是自闭合元素且不是void元素，推入栈中
    if (token.type() != TokenType::CLOSE_SELF && !is_void_element(token.name())) {
        push_element(raw_ptr);
    }
}

void TreeBuilder::process_end_tag(const Token& token) {
    std::string_view tag_name = token.name();

    // 检查是否为void元素的错误闭合标签
    if (is_void_element(tag_name)) {
        parse_error(ErrorCode::VoidElementClose, "Void element should not have closing tag: " + std::string(tag_name));
        return;
    }

    // 如果栈为空，说明没有对应的开始标签
    if (m_element_stack.empty()) {
        parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: " + std::string(tag_name));
        return;
    }

    // 查找匹配的开始标签（从栈顶向下查找）
    auto it = std::ranges::find_if(std::ranges::reverse_view(m_element_stack), [tag_name](const Element* elem) { return elem->tag_name() == tag_name; });

    if (it != m_element_stack.rend()) {
        // 找到匹配的标签，关闭到这个位置的所有元素
        close_elements_until(tag_name);
    } else {
        parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: " + std::string(tag_name));
    }
}

void TreeBuilder::process_text(const Token& token) const {
    const std::string_view text = token.value();
    if (text.empty()) {
        return;
    }
    insert_text(text);
}

std::unique_ptr<Element> TreeBuilder::create_element(const Token& token) {
    auto element = std::make_unique<Element>(token.name());
    for (const auto& attr : token.attrs()) {
        element->add_attribute(attr.m_name, attr.m_value);
    }
    return element;
}

void TreeBuilder::insert_element(std::unique_ptr<Element> element) const {
    if (m_element_stack.empty()) {
        // 如果栈为空，直接添加到document
        m_document->add_child(std::move(element));
    } else {
        // 添加到当前元素
        Element* current = current_element();
        current->add_child(std::move(element));
    }
}

void TreeBuilder::insert_text(std::string_view text) const {
    auto text_node = std::make_unique<TextNode>(text);
    if (m_element_stack.empty()) {
        m_document->add_child(std::move(text_node));
    } else {
        Element* parent = current_element();
        parent->add_child(std::move(text_node));
    }
}

void TreeBuilder::push_element(Element* element) {
    m_element_stack.push_back(element);
}

Element* TreeBuilder::pop_element() {
    if (m_element_stack.empty()) {
        return nullptr;
    }

    Element* element = m_element_stack.back();
    m_element_stack.pop_back();
    return element;
}

Element* TreeBuilder::current_element() const {
    if (m_element_stack.empty()) {
        return nullptr;
    }
    return m_element_stack.back();
}

bool TreeBuilder::should_close_element(std::string_view tag_name) const {
    if (Element* current = current_element()) {
        return current->tag_name() == tag_name;
    }
    return false;
}

void TreeBuilder::close_elements_until(std::string_view tag_name) {
    // 从栈顶开始，关闭元素直到找到匹配的标签
    while (!m_element_stack.empty()) {
        Element* element = m_element_stack.back();

        // 先弹出元素
        m_element_stack.pop_back();

        if (element->tag_name() == tag_name) {
            // 找到匹配的标签，停止关闭
            break;
        }

        // 如果不匹配，记录自动关闭的警告
        parse_error(ErrorCode::MismatchedTag, "Auto-closing unclosed tag: " + std::string(element->tag_name()));
    }
}

void TreeBuilder::parse_error(const ErrorCode code, const std::string& message) {
    ParseError error(code, message, 0);
    m_errors.push_back(std::move(error));
}

}  // namespace hps