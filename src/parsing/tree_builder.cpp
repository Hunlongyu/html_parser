#include "hps/parsing/tree_builder.hpp"

#include "hps/core/comment_node.hpp"
#include "hps/core/document.hpp"
#include "hps/core/text_node.hpp"
#include "hps/parsing/token.hpp"
#include "hps/utils/string_utils.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>

namespace hps {

TreeBuilder::TreeBuilder(const std::shared_ptr<Document>& document, const Options& options)
    : m_document(document),
      m_options(options) {
    assert(m_document != nullptr);
    m_element_stack.reserve(32);
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
                process_text(token);
                break;
            case TokenType::COMMENT:
                process_comment(token);
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
    while (!m_element_stack.empty()) {
        const auto element = m_element_stack.back();
        m_element_stack.pop_back();
        parse_error(ErrorCode::UnclosedTag, "Unclosed tag: " + std::string(element->tag_name()));
    }

    return true;
}

const std::vector<HPSError>& TreeBuilder::errors() const noexcept {
    return m_errors;
}

void TreeBuilder::process_start_tag(const Token& token) {
    if (token.name() == "br") {
        if (m_options.br_handling == BRHandling::InsertNewline) {
            insert_text("\n");
        } else if (m_options.br_handling == BRHandling::InsertCustom) {
            insert_text(m_options.br_text);
        }
    }

    auto element = create_element(token);
    // 保存原始指针用于栈操作
    Element* element_ptr = element.get();
    
    // 将所有权移交给 DOM 树
    insert_element(std::move(element));

    if (!m_options.is_void_element(std::string(token.name())) && token.type() != TokenType::CLOSE_SELF) {
        push_element(element_ptr);
    }
}

void TreeBuilder::process_end_tag(const Token& token) {
    std::string_view tag_name = token.name();
    if (m_options.is_void_element(std::string(token.name()))) {
        parse_error(ErrorCode::VoidElementClose, "Void element should not have closing tag: " + std::string(tag_name));
        return;
    }
    if (m_element_stack.empty()) {
        parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: " + std::string(tag_name));
        return;
    }
    const auto it = std::ranges::find_if(std::ranges::reverse_view(m_element_stack), [tag_name](const Element* elem) { return elem->tag_name() == tag_name; });
    if (it != m_element_stack.rend()) {
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
    std::string processed_text(text);

    switch (m_options.text_processing_mode) {
        case TextProcessingMode::Raw:
            break;
        case TextProcessingMode::Decode:
            processed_text = decode_html_entities(processed_text);
            break;
    }

    switch (m_options.whitespace_mode) {
        case WhitespaceMode::Preserve:
            insert_text(processed_text);
            break;
        case WhitespaceMode::Normalize:
            insert_text(normalize_whitespace(processed_text));
            break;
        case WhitespaceMode::Trim:
            insert_text(trim_whitespace(processed_text));
            break;
        case WhitespaceMode::Remove:
            break;
    }
}

void TreeBuilder::process_comment(const Token& token) const {
    const std::string_view comment = token.value();
    if (comment.empty()) {
        return;
    }

    switch (m_options.comment_mode) {
        case CommentMode::Preserve:
            insert_comment(comment);
            break;
        case CommentMode::Remove:
        case CommentMode::ProcessOnly:
            break;
    }
}

std::unique_ptr<Element> TreeBuilder::create_element(const Token& token) {
    auto element = std::make_unique<Element>(token.name());
    for (const auto& attr : token.attrs()) {
        element->add_attribute(attr.name, attr.value);
    }
    return element;
}

void TreeBuilder::insert_element(std::unique_ptr<Element> element) const {
    if (m_element_stack.empty()) {
        m_document->add_child(std::move(element));
    } else {
        const auto current = current_element();
        current->add_child(std::move(element));
    }
}

void TreeBuilder::insert_text(std::string_view text) const {
    auto text_node = std::make_unique<TextNode>(text);
    if (m_element_stack.empty()) {
        m_document->add_child(std::move(text_node));
    } else {
        const auto parent = current_element();
        parent->add_child(std::move(text_node));
    }
}

void TreeBuilder::insert_comment(std::string_view comment) const {
    auto comment_node = std::make_unique<CommentNode>(comment);
    if (m_element_stack.empty()) {
        m_document->add_child(std::move(comment_node));
    } else {
        const auto parent = current_element();
        parent->add_child(std::move(comment_node));
    }
}

void TreeBuilder::push_element(Element* element) {
    m_element_stack.push_back(element);
}

Element* TreeBuilder::current_element() const {
    if (m_element_stack.empty()) {
        return nullptr;
    }
    return m_element_stack.back();
}

void TreeBuilder::close_elements_until(const std::string_view tag_name) {
    while (!m_element_stack.empty()) {
        const auto element = m_element_stack.back();
        m_element_stack.pop_back();
        if (element->tag_name() == tag_name) {
            break;
        }
        parse_error(ErrorCode::MismatchedTag, "Auto-closing unclosed tag: " + std::string(element->tag_name()));
    }
}

void TreeBuilder::parse_error(const ErrorCode code, const std::string& message) {
    HPSError error(code, message, 0);
    m_errors.push_back(std::move(error));
}

}  // namespace hps
