#include "hps/parsing/tree_builder.hpp"

#include "hps/core/comment_node.hpp"
#include "hps/core/document.hpp"
#include "hps/core/text_node.hpp"
#include "hps/parsing/token.hpp"
#include "hps/utils/string_utils.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <ranges>

namespace hps {

namespace {

[[nodiscard]] constexpr auto sorted_string_view_array(auto array) {
    return array;
}

[[nodiscard]] auto clone_element_shallow(const Element& source) -> std::unique_ptr<Element> {
    auto clone = std::make_unique<Element>(source.tag_name(), source.namespace_kind());
    for (const auto& attribute : source.attributes()) {
        clone->add_attribute(attribute.name(), attribute.value(), attribute.has_value());
    }
    return clone;
}

}  // namespace

TreeBuilder::TreeBuilder(const std::shared_ptr<Document>& document, const Options& options)
    : m_document(document),
      m_options(options) {
    assert(m_document != nullptr);
    m_element_stack.reserve(32);
    m_ignored_element_stack.reserve(8);
}

TreeBuilder::TreeBuilder(
    const std::shared_ptr<Document>& document,
    const Options& options,
    Element* fragment_context)
    : TreeBuilder(document, options) {
    m_fragment_context = fragment_context;
    if (m_fragment_context != nullptr) {
        push_element(m_fragment_context);
        m_stack_floor = m_element_stack.size();
    }
}

bool TreeBuilder::process_token(const Token& token, const size_t position) {
    m_last_position = position;

    try {
        if (!m_ignored_element_stack.empty()) {
            switch (token.type()) {
                case TokenType::OPEN:
                case TokenType::CLOSE_SELF:
                    if (!m_options.is_void_element(token.name()) && token.type() != TokenType::CLOSE_SELF) {
                        m_ignored_element_stack.emplace_back(token.name());
                    }
                    return true;
                case TokenType::CLOSE:
                    if (token.name() == m_ignored_element_stack.back()) {
                        m_ignored_element_stack.pop_back();
                    }
                    return true;
                case TokenType::TEXT:
                case TokenType::COMMENT:
                case TokenType::DONE:
                case TokenType::FORCE_QUIRKS:
                case TokenType::DOCTYPE:
                    return true;
            }
        }

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
    } catch (const HPSException&) {
        throw;
    } catch (const std::exception& e) {
        parse_error(ErrorCode::UnknownError, e.what(), position);
        return false;
    }
}

bool TreeBuilder::finish() {
    while (m_element_stack.size() > m_stack_floor) {
        const auto element = m_element_stack.back();
        m_element_stack.pop_back();
        if (!can_omit_end_tag_at_eof(element->tag_name())) {
            parse_error(ErrorCode::UnclosedTag, "Unclosed tag: " + std::string(element->tag_name()), m_last_position);
        }
    }

    return true;
}

const std::vector<HPSError>& TreeBuilder::errors() const noexcept {
    return m_errors;
}

std::vector<HPSError> TreeBuilder::consume_errors() {
    return std::move(m_errors);
}

void TreeBuilder::process_start_tag(const Token& token) {
    if (m_fragment_context == nullptr) {
        if (equals_ignore_case(token.name(), "html")) {
            process_html_start_tag(token);
            return;
        }
        if (equals_ignore_case(token.name(), "head")) {
            process_head_start_tag(token);
            return;
        }
        if (equals_ignore_case(token.name(), "body")) {
            process_body_start_tag(token);
            return;
        }

        if (is_head_content_tag(token.name()) && m_body_element == nullptr) {
            ensure_head_element();
        } else {
            if (current_element() == m_head_element && !m_head_closed) {
                close_head_element_if_open();
            }
            ensure_body_element();
        }
    } else {
    }

    if (equals_ignore_case(token.name(), "form") &&
        find_open_element("form", false) != nullptr) {
        parse_error(ErrorCode::InvalidNesting, "Unexpected nested <form>", m_last_position);
        return;
    }
    if (equals_ignore_case(token.name(), "a") &&
        find_open_element("a", false) != nullptr) {
        parse_error(ErrorCode::InvalidNesting, "Unexpected nested <a>", m_last_position);
        if (!try_recover_formatting_end_tag("a")) {
            close_elements_until("a", false);
        }
    }

    if (!equals_ignore_case(token.name(), "col")) {
        close_colgroup_for_non_col_token();
    }

    const bool foster_parent_element = should_foster_parent_element(token.name());
    check_implicit_close(token.name());
    if (!foster_parent_element) {
        prepare_table_context_for_start_tag(token.name());
    }
    prepare_select_context_for_start_tag(token.name());

    const size_t content_depth =
        static_cast<size_t>(std::ranges::count_if(m_element_stack, [this](const Element* element) {
            return element != m_html_element && element != m_head_element && element != m_body_element;
        }));
    const size_t next_depth = content_depth + 1;
    if (next_depth > m_options.max_depth) {
        parse_error(ErrorCode::TooDeep, "Nesting depth limit exceeded at <" + std::string(token.name()) + ">", m_last_position);
        if (!m_options.is_void_element(token.name()) && token.type() != TokenType::CLOSE_SELF) {
            m_ignored_element_stack.emplace_back(token.name());
        }
        return;
    }

    if (token.name() == "br") {
        if (m_options.br_handling == BRHandling::InsertNewline) {
            if (foster_parent_element) {
                const auto [parent, before] = foster_parent_insertion_point();
                insert_text_before("\n", parent, before);
            } else {
                insert_text("\n");
            }
        } else if (m_options.br_handling == BRHandling::InsertCustom) {
            if (foster_parent_element) {
                const auto [parent, before] = foster_parent_insertion_point();
                insert_text_before(m_options.br_text, parent, before);
            } else {
                insert_text(m_options.br_text);
            }
        }
    }

    auto element = create_element(token, namespace_for_start_tag(token.name()));
    Element* element_ptr = element.get();
    if (foster_parent_element) {
        const auto [parent, before] = foster_parent_insertion_point();
        element_ptr = const_cast<Element*>(insert_node_before(std::move(element), parent, before)->as_element());
    } else {
        insert_element(std::move(element));
    }

    if (!m_options.is_void_element(std::string(token.name())) && token.type() != TokenType::CLOSE_SELF) {
        push_element(element_ptr);
    }
}

void TreeBuilder::process_html_start_tag(const Token& token) {
    if (!m_html_element) {
        auto html_element = create_element(token);
        m_html_element    = const_cast<Element*>(insert_node(std::move(html_element), m_document.get())->as_element());
        if (token.type() != TokenType::CLOSE_SELF) {
            push_if_absent(m_html_element);
        }
        return;
    }

    merge_token_attributes(*m_html_element, token);
}

void TreeBuilder::process_head_start_tag(const Token& token) {
    ensure_html_element();

    if (m_body_element != nullptr) {
        parse_error(ErrorCode::InvalidNesting, "Unexpected <head> after <body>", m_last_position);
        return;
    }

    if (!m_head_element) {
        auto head_element = create_element(token);
        m_head_element    = const_cast<Element*>(insert_node(std::move(head_element), m_html_element)->as_element());
    } else {
        merge_token_attributes(*m_head_element, token);
    }

    m_head_closed = false;
    if (token.type() != TokenType::CLOSE_SELF) {
        push_if_absent(m_head_element);
    }
}

void TreeBuilder::process_body_start_tag(const Token& token) {
    ensure_html_element();

    if (!m_head_element) {
        ensure_head_element();
    }
    close_head_element_if_open();

    if (!m_body_element) {
        auto body_element = create_element(token);
        m_body_element    = const_cast<Element*>(insert_node(std::move(body_element), m_html_element)->as_element());
    } else {
        merge_token_attributes(*m_body_element, token);
    }

    if (token.type() != TokenType::CLOSE_SELF) {
        push_if_absent(m_body_element);
    }
}

void TreeBuilder::process_end_tag(const Token& token) {
    std::string_view tag_name = token.name();

    if (m_fragment_context == nullptr && equals_ignore_case(tag_name, "head")) {
        close_head_element_if_open();
        return;
    }
    if (m_fragment_context == nullptr && equals_ignore_case(tag_name, "body")) {
        close_head_element_if_open();
        if (!m_body_element) {
            return;
        }
        if (is_on_stack(m_body_element)) {
            close_elements_until("body", false);
        }
        return;
    }
    if (m_fragment_context == nullptr && equals_ignore_case(tag_name, "html")) {
        close_head_element_if_open();
        if (m_body_element && is_on_stack(m_body_element)) {
            close_elements_until("body", false);
        }
        if (m_html_element && is_on_stack(m_html_element)) {
            close_elements_until("html", false);
        }
        return;
    }

    if (handle_table_end_tag(tag_name)) {
        return;
    }
    if (handle_select_end_tag(tag_name)) {
        return;
    }

    if (m_options.is_void_element(std::string(token.name()))) {
        return;
    }

    if (m_element_stack.empty()) {
        parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: " + std::string(tag_name));
        return;
    }
    if (try_recover_formatting_end_tag(tag_name)) {
        return;
    }
    if (find_open_element(tag_name, false) != nullptr) {
        close_elements_until(tag_name);
    } else {
        parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: " + std::string(tag_name));
    }
}

void TreeBuilder::process_text(const Token& token) {
    const std::string_view text = token.value();
    if (text.empty()) {
        return;
    }

    if (!is_all_whitespace(text)) {
        close_colgroup_for_non_col_token();
    }

    if (m_fragment_context == nullptr && current_element() == m_head_element && !is_all_whitespace(text)) {
        close_head_element_if_open();
    }
    if (m_fragment_context == nullptr &&
        (current_element() == nullptr || current_element() == m_html_element)) {
        ensure_body_element();
    }

    std::string processed_text(text);

    switch (m_options.text_processing_mode) {
        case TextProcessingMode::Raw:
            break;
        case TextProcessingMode::Decode:
            processed_text = decode_html_entities(processed_text);
            break;
    }

    if (m_options.decode_entities && m_options.text_processing_mode == TextProcessingMode::Raw) {
        processed_text = decode_html_entities(processed_text);
    }

    std::string final_text;
    switch (m_options.whitespace_mode) {
        case WhitespaceMode::Preserve:
            final_text = processed_text;
            break;
        case WhitespaceMode::Normalize:
            final_text = normalize_whitespace(processed_text);
            break;
        case WhitespaceMode::Trim:
            final_text = std::string(trim_whitespace(processed_text));
            break;
        case WhitespaceMode::Remove:
            return;
    }

    if (final_text.empty()) {
        return;
    }

    if (should_foster_parent_text() && !is_all_whitespace(final_text)) {
        const auto [parent, before] = foster_parent_insertion_point();
        insert_text_before(final_text, parent, before);
        return;
    }

    insert_text(final_text);
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
    return create_element(token, NamespaceKind::Html);
}

std::unique_ptr<Element> TreeBuilder::create_element(
    const Token& token,
    const NamespaceKind namespace_kind) {
    auto element = std::make_unique<Element>(token.name(), namespace_kind);
    merge_token_attributes(*element, token);
    return element;
}

void TreeBuilder::merge_token_attributes(Element& element, const Token& token) {
    for (const auto& attr : token.attrs()) {
        element.add_attribute(attr.name, attr.value, attr.has_value);
    }
}

void TreeBuilder::insert_element(std::unique_ptr<Element> element) const {
    if (m_element_stack.empty()) {
        m_document->add_child(std::move(element));
    } else {
        const auto current = current_element();
        current->add_child(std::move(element));
    }
}

Node* TreeBuilder::insert_node(std::unique_ptr<Node> child, Node* parent) const {
    if (!child) {
        return nullptr;
    }

    if (parent == nullptr || parent->is_document()) {
        return m_document->add_child(std::move(child));
    }

    if (auto* parent_element = const_cast<Element*>(parent->as_element())) {
        return parent_element->add_child(std::move(child));
    }
    return nullptr;
}

Node* TreeBuilder::insert_node_before(
    std::unique_ptr<Node> child,
    Node* parent,
    const Node* before) const {
    if (!child) {
        return nullptr;
    }

    if (parent == nullptr || parent->is_document()) {
        return m_document->insert_child_before(std::move(child), before);
    }

    if (auto* parent_element = const_cast<Element*>(parent->as_element())) {
        return parent_element->insert_child_before(std::move(child), before);
    }
    return nullptr;
}

void TreeBuilder::insert_text(std::string_view text) const {
    if (text.empty()) {
        return;
    }

    Node* parent;
    if (m_element_stack.empty()) {
        parent = m_document.get();
    } else {
        parent = current_element();
    }

    if (parent) {
        if (Node* last = parent->last_child_mut()) {
            if (last->type() == NodeType::Text) {
                dynamic_cast<TextNode*>(last)->append_text(text);
                return;
            }
        }
    }

    auto text_node = std::make_unique<TextNode>(text);
    if (m_element_stack.empty()) {
        m_document->add_child(std::move(text_node));
    } else {
        const auto element = current_element();
        element->add_child(std::move(text_node));
    }
}

void TreeBuilder::insert_text_before(
    std::string_view text,
    Node* parent,
    const Node* before) const {
    if (text.empty() || parent == nullptr) {
        return;
    }

    Node* previous = nullptr;
    if (before != nullptr) {
        previous = const_cast<Node*>(before->previous_sibling());
    } else {
        previous = parent->last_child_mut();
    }

    if (previous != nullptr && previous->type() == NodeType::Text) {
        dynamic_cast<TextNode*>(previous)->append_text(text);
        return;
    }

    auto text_node = std::make_unique<TextNode>(text);
    insert_node_before(std::move(text_node), parent, before);
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

void TreeBuilder::push_if_absent(Element* element) {
    if (!element || is_on_stack(element)) {
        return;
    }
    push_element(element);
}

Element* TreeBuilder::current_element() const {
    if (m_element_stack.empty()) {
        return nullptr;
    }
    return m_element_stack.back();
}

bool TreeBuilder::is_on_stack(const Element* element) const noexcept {
    return element != nullptr &&
           std::ranges::find(m_element_stack, element) != m_element_stack.end();
}

void TreeBuilder::close_elements_until(const std::string_view tag_name, const bool report_auto_close_errors) {
    while (m_element_stack.size() > m_stack_floor) {
        const auto element = m_element_stack.back();
        m_element_stack.pop_back();
        if (equals_ignore_case(element->tag_name(), tag_name)) {
            break;
        }
        if (report_auto_close_errors && !can_omit_end_tag_at_eof(element->tag_name())) {
            parse_error(ErrorCode::MismatchedTag, "Auto-closing unclosed tag: " + std::string(element->tag_name()));
        }
    }
}

void TreeBuilder::parse_error(const ErrorCode code, const std::string& message, const size_t position) {
    const auto location = Location::from_position(m_document->source_html(), position);
    m_errors.emplace_back(code, message, location);
    if (m_options.error_handling == ErrorHandlingMode::Strict) {
        throw HPSException(code, message, location);
    }
}

void TreeBuilder::check_implicit_close(const std::string_view tag_name) {
    while (m_element_stack.size() > m_stack_floor) {
        const auto             current     = current_element();
        const std::string_view current_tag = current->tag_name();

        // <p> implies closing by block elements
        static constexpr std::array<std::string_view, 26> p_closers = {"address", "article", "aside", "blockquote", "div", "dl", "fieldset", "footer", "form", "h1", "h2", "h3", "h4", "h5", "h6", "header", "hgroup", "hr", "main", "nav", "ol", "p", "pre", "section", "table", "ul"};
        const bool closes_paragraph =
            current_tag == "p" && std::ranges::binary_search(p_closers, tag_name);
        const bool closes_list_item =
            (current_tag == "li" && tag_name == "li") ||
            ((current_tag == "dd" || current_tag == "dt") &&
             (tag_name == "dd" || tag_name == "dt"));
        const bool closes_button =
            equals_ignore_case(current_tag, "button") &&
            equals_ignore_case(tag_name, "button");
        const bool closes_table_cell =
            is_table_cell_tag(current_tag) &&
            (is_table_cell_tag(tag_name) || equals_ignore_case(tag_name, "tr") ||
             is_table_section_tag(tag_name));
        const bool closes_table_row =
            equals_ignore_case(current_tag, "tr") &&
            (equals_ignore_case(tag_name, "tr") || is_table_section_tag(tag_name) ||
             equals_ignore_case(tag_name, "table"));
        const bool closes_table_section =
            is_table_section_tag(current_tag) &&
            (is_table_section_tag(tag_name) || equals_ignore_case(tag_name, "table"));
        const bool should_pop_current =
            closes_paragraph || closes_list_item || closes_button || closes_table_cell ||
            closes_table_row || closes_table_section;

        if (should_pop_current) {
            m_element_stack.pop_back();
            continue;
        }
        break;
    }
}

void TreeBuilder::ensure_html_element() {
    if (m_html_element != nullptr) {
        return;
    }

    auto html_element = std::make_unique<Element>("html");
    m_html_element    = const_cast<Element*>(insert_node(std::move(html_element), m_document.get())->as_element());
    push_if_absent(m_html_element);
}

void TreeBuilder::ensure_head_element() {
    ensure_html_element();

    if (!m_head_element) {
        auto head_element = std::make_unique<Element>("head");
        m_head_element    = const_cast<Element*>(insert_node(std::move(head_element), m_html_element)->as_element());
    }

    m_head_closed = false;
    if (current_element() == nullptr || current_element() == m_html_element) {
        push_if_absent(m_head_element);
    }
}

void TreeBuilder::ensure_body_element() {
    ensure_html_element();

    if (!m_head_element) {
        auto head_element = std::make_unique<Element>("head");
        m_head_element    = const_cast<Element*>(insert_node(std::move(head_element), m_html_element)->as_element());
        m_head_closed = true;
    } else if (!m_head_closed) {
        close_head_element_if_open();
    }

    if (!m_body_element) {
        auto body_element = std::make_unique<Element>("body");
        m_body_element    = const_cast<Element*>(insert_node(std::move(body_element), m_html_element)->as_element());
    }

    if (current_element() == nullptr || current_element() == m_html_element) {
        push_if_absent(m_body_element);
    }
}

void TreeBuilder::close_head_element_if_open() {
    if (m_head_element == nullptr || m_head_closed) {
        return;
    }

    if (is_on_stack(m_head_element)) {
        close_elements_until("head", false);
    }
    m_head_closed = true;
}

void TreeBuilder::prepare_table_context_for_start_tag(const std::string_view tag_name) {
    if (is_table_structure_tag(tag_name)) {
        close_foster_parented_elements_before_table_token();
    }

    if (equals_ignore_case(tag_name, "caption")) {
        close_open_table_content_before_container("caption");
        return;
    }

    if (equals_ignore_case(tag_name, "colgroup")) {
        close_open_table_content_before_container("colgroup");
        return;
    }

    if (equals_ignore_case(tag_name, "col")) {
        close_open_table_content_before_container("colgroup", false);
        ensure_colgroup();
        return;
    }

    if (is_table_section_tag(tag_name) || equals_ignore_case(tag_name, "tr") ||
        is_table_cell_tag(tag_name)) {
        if (find_open_element("caption", false) != nullptr) {
            close_elements_until("caption", false);
        }
        if (find_open_element("colgroup", false) != nullptr) {
            close_elements_until("colgroup", false);
        }
    }

    if (equals_ignore_case(tag_name, "tr")) {
        ensure_table_section();
        return;
    }

    if (is_table_cell_tag(tag_name)) {
        ensure_table_row();
    }
}

void TreeBuilder::prepare_select_context_for_start_tag(const std::string_view tag_name) {
    if (equals_ignore_case(tag_name, "option")) {
        if (find_open_in_select_scope("option") != nullptr) {
            close_elements_until("option", false);
        }
        return;
    }

    if (equals_ignore_case(tag_name, "optgroup")) {
        if (find_open_in_select_scope("option") != nullptr) {
            close_elements_until("option", false);
        }
        if (find_open_in_select_scope("optgroup") != nullptr) {
            close_elements_until("optgroup", false);
        }
        return;
    }

    if (equals_ignore_case(tag_name, "select")) {
        if (find_open_in_select_scope("option") != nullptr) {
            close_elements_until("option", false);
        }
        if (find_open_in_select_scope("optgroup") != nullptr) {
            close_elements_until("optgroup", false);
        }
        if (find_open_in_select_scope("select") != nullptr) {
            close_elements_until("select", false);
        }
    }
}

bool TreeBuilder::handle_table_end_tag(const std::string_view tag_name) {
    if (equals_ignore_case(tag_name, "table") ||
        equals_ignore_case(tag_name, "tr") ||
        is_table_section_tag(tag_name) ||
        is_table_cell_tag(tag_name) ||
        equals_ignore_case(tag_name, "caption") ||
        equals_ignore_case(tag_name, "colgroup")) {
        close_foster_parented_elements_before_table_token();
    }

    if (equals_ignore_case(tag_name, "caption") || equals_ignore_case(tag_name, "colgroup")) {
        if (find_open_element(tag_name, false) == nullptr) {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: " + std::string(tag_name));
            return true;
        }
        close_elements_until(tag_name, false);
        return true;
    }

    if (is_table_cell_tag(tag_name)) {
        if (find_open_element(tag_name, false) == nullptr) {
            return false;
        }
        close_elements_until(tag_name, false);
        return true;
    }

    if (equals_ignore_case(tag_name, "tr")) {
        if (find_open_table_cell() != nullptr) {
            close_elements_until(find_open_table_cell()->tag_name(), false);
        }
        if (find_open_table_row() == nullptr) {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: tr");
            return true;
        }
        close_elements_until("tr", false);
        return true;
    }

    if (is_table_section_tag(tag_name)) {
        if (find_open_table_cell() != nullptr) {
            close_elements_until(find_open_table_cell()->tag_name(), false);
        }
        if (find_open_table_row() != nullptr) {
            close_elements_until("tr", false);
        }
        if (find_open_element(tag_name, false) == nullptr) {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: " + std::string(tag_name));
            return true;
        }
        close_elements_until(tag_name, false);
        return true;
    }

    if (equals_ignore_case(tag_name, "table")) {
        if (find_open_element("caption", false) != nullptr) {
            close_elements_until("caption", false);
        }
        if (find_open_element("colgroup", false) != nullptr) {
            close_elements_until("colgroup", false);
        }
        if (find_open_table_cell() != nullptr) {
            close_elements_until(find_open_table_cell()->tag_name(), false);
        }
        if (find_open_table_row() != nullptr) {
            close_elements_until("tr", false);
        }
        if (find_open_table_section() != nullptr) {
            close_elements_until(find_open_table_section()->tag_name(), false);
        }
        if (find_open_element("table", false) == nullptr) {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: table");
            return true;
        }
        close_elements_until("table", false);
        return true;
    }

    return false;
}

bool TreeBuilder::handle_select_end_tag(const std::string_view tag_name) {
    if (equals_ignore_case(tag_name, "option")) {
        if (find_open_in_select_scope("option") == nullptr) {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: option");
            return true;
        }
        close_elements_until("option", false);
        return true;
    }

    if (equals_ignore_case(tag_name, "optgroup")) {
        if (find_open_in_select_scope("option") != nullptr) {
            close_elements_until("option", false);
        }
        if (find_open_in_select_scope("optgroup") == nullptr) {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: optgroup");
            return true;
        }
        close_elements_until("optgroup", false);
        return true;
    }

    if (equals_ignore_case(tag_name, "select")) {
        if (find_open_in_select_scope("option") != nullptr) {
            close_elements_until("option", false);
        }
        if (find_open_in_select_scope("optgroup") != nullptr) {
            close_elements_until("optgroup", false);
        }
        if (find_open_in_select_scope("select") == nullptr) {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: select");
            return true;
        }
        close_elements_until("select", false);
        return true;
    }

    return false;
}

void TreeBuilder::close_open_table_content_before_container(
    const std::string_view tag_name,
    const bool close_matching_tag) {
    if (find_open_table_cell() != nullptr) {
        close_elements_until(find_open_table_cell()->tag_name(), false);
    }
    if (find_open_element("tr", false) != nullptr) {
        close_elements_until("tr", false);
    }
    if (find_open_table_section() != nullptr) {
        close_elements_until(find_open_table_section()->tag_name(), false);
    }

    for (const std::string_view container : {std::string_view("caption"), std::string_view("colgroup")}) {
        if (!equals_ignore_case(container, tag_name) && find_open_element(container, false) != nullptr) {
            close_elements_until(container, false);
        }
    }

    if (close_matching_tag && find_open_element(tag_name, false) != nullptr) {
        close_elements_until(tag_name, false);
    }
}

void TreeBuilder::ensure_table_section(const std::string_view tag_name) {
    if (find_open_element("table") == nullptr || find_open_table_section() != nullptr) {
        return;
    }

    auto section = std::make_unique<Element>(tag_name);
    auto* section_ptr =
        const_cast<Element*>(insert_node(std::move(section), find_open_element("table"))->as_element());
    push_if_absent(section_ptr);
}

void TreeBuilder::ensure_table_row() {
    ensure_table_section();
    if (find_open_table_row() != nullptr) {
        return;
    }

    Element* section = find_open_table_section();
    if (section == nullptr) {
        return;
    }

    auto row = std::make_unique<Element>("tr");
    auto* row_ptr =
        const_cast<Element*>(insert_node(std::move(row), section)->as_element());
    push_if_absent(row_ptr);
}

void TreeBuilder::ensure_colgroup() {
    if (find_open_element("table") == nullptr || find_open_element("colgroup") != nullptr) {
        return;
    }

    auto colgroup = std::make_unique<Element>("colgroup");
    auto* colgroup_ptr =
        const_cast<Element*>(insert_node(std::move(colgroup), find_open_element("table"))->as_element());
    push_if_absent(colgroup_ptr);
}

void TreeBuilder::close_colgroup_for_non_col_token() {
    if (current_element() == nullptr) {
        return;
    }
    if (!equals_ignore_case(current_element()->tag_name(), "colgroup")) {
        return;
    }
    close_elements_until("colgroup", false);
}

bool TreeBuilder::is_head_content_tag(const std::string_view tag_name) noexcept {
    static constexpr auto head_content_tags = sorted_string_view_array(std::array<std::string_view, 11>{
        "base",
        "basefont",
        "bgsound",
        "link",
        "meta",
        "noframes",
        "noscript",
        "script",
        "style",
        "template",
        "title",
    });
    return std::ranges::binary_search(head_content_tags, tag_name);
}

bool TreeBuilder::is_table_section_tag(const std::string_view tag_name) noexcept {
    static constexpr auto table_section_tags = sorted_string_view_array(
        std::array<std::string_view, 3>{"tbody", "tfoot", "thead"});
    return std::ranges::binary_search(table_section_tags, tag_name);
}

bool TreeBuilder::is_table_cell_tag(const std::string_view tag_name) noexcept {
    static constexpr auto table_cell_tags =
        sorted_string_view_array(std::array<std::string_view, 2>{"td", "th"});
    return std::ranges::binary_search(table_cell_tags, tag_name);
}

bool TreeBuilder::is_table_structure_tag(const std::string_view tag_name) noexcept {
    static constexpr auto table_structure_tags = sorted_string_view_array(
        std::array<std::string_view, 9>{"caption", "col", "colgroup", "table", "tbody", "td", "tfoot", "th", "thead"});
    return std::ranges::binary_search(table_structure_tags, tag_name) ||
           equals_ignore_case(tag_name, "tr");
}

bool TreeBuilder::is_table_container_tag(const std::string_view tag_name) noexcept {
    static constexpr auto table_container_tags = sorted_string_view_array(
        std::array<std::string_view, 2>{"caption", "colgroup"});
    return std::ranges::binary_search(table_container_tags, tag_name);
}

bool TreeBuilder::is_adoption_formatting_tag(const std::string_view tag_name) noexcept {
    static constexpr auto formatting_tags = sorted_string_view_array(std::array<std::string_view, 15>{
        "a",
        "b",
        "big",
        "code",
        "em",
        "font",
        "i",
        "nobr",
        "s",
        "small",
        "strike",
        "strong",
        "tt",
        "u",
        "span",
    });
    return std::ranges::binary_search(formatting_tags, tag_name);
}

NamespaceKind TreeBuilder::current_insertion_namespace() const noexcept {
    const auto* current = current_element();
    if (current == nullptr) {
        return NamespaceKind::Html;
    }
    if (current->namespace_kind() == NamespaceKind::Svg &&
        equals_ignore_case(current->tag_name(), "foreignobject")) {
        return NamespaceKind::Html;
    }
    return current->namespace_kind();
}

NamespaceKind TreeBuilder::namespace_for_start_tag(const std::string_view tag_name) const noexcept {
    const auto inherited_namespace = current_insertion_namespace();
    if (equals_ignore_case(tag_name, "svg")) {
        return NamespaceKind::Svg;
    }
    if (equals_ignore_case(tag_name, "math")) {
        return NamespaceKind::MathML;
    }
    if (inherited_namespace != NamespaceKind::Html) {
        return inherited_namespace;
    }
    return NamespaceKind::Html;
}

bool TreeBuilder::can_omit_end_tag_at_eof(const std::string_view tag_name) noexcept {
    static constexpr auto optional_end_tags = sorted_string_view_array(std::array<std::string_view, 18>{
        "body",
        "caption",
        "colgroup",
        "dd",
        "dt",
        "head",
        "html",
        "li",
        "optgroup",
        "option",
        "p",
        "rb",
        "rp",
        "rt",
        "rtc",
        "tbody",
        "td",
        "tfoot",
    });
    static constexpr auto optional_end_tags_tail = sorted_string_view_array(std::array<std::string_view, 3>{
        "th",
        "thead",
        "tr",
    });
    return std::ranges::binary_search(optional_end_tags, tag_name) ||
           std::ranges::binary_search(optional_end_tags_tail, tag_name);
}

bool TreeBuilder::is_all_whitespace(const std::string_view text) noexcept {
    return std::ranges::all_of(text, is_whitespace);
}

bool TreeBuilder::should_foster_parent_text() const noexcept {
    const auto* current = current_element();
    if (current == nullptr) {
        return false;
    }

    const std::string_view current_tag = current->tag_name();
    return equals_ignore_case(current_tag, "table") ||
           is_table_section_tag(current_tag) ||
           equals_ignore_case(current_tag, "tr");
}

bool TreeBuilder::should_foster_parent_element(const std::string_view tag_name) const noexcept {
    if (is_table_structure_tag(tag_name)) {
        return false;
    }

    const auto* current = current_element();
    if (current == nullptr) {
        return false;
    }

    const std::string_view current_tag = current->tag_name();
    return equals_ignore_case(current_tag, "table") ||
           is_table_section_tag(current_tag) ||
           equals_ignore_case(current_tag, "tr");
}

std::pair<Node*, const Node*> TreeBuilder::foster_parent_insertion_point() const noexcept {
    Element* table = find_open_element("table");
    if (table == nullptr) {
        return {current_element() != nullptr ? static_cast<Node*>(current_element())
                                             : static_cast<Node*>(m_document.get()),
                nullptr};
    }

    if (table == m_fragment_context) {
        return {table, table->first_child()};
    }

    Node* parent = const_cast<Node*>(table->parent());
    if (parent == nullptr) {
        return {table, table->first_child()};
    }
    return {parent, table};
}

void TreeBuilder::close_foster_parented_elements_before_table_token() noexcept {
    Element* table = find_open_element("table");
    if (table == nullptr) {
        return;
    }

    while (m_element_stack.size() > m_stack_floor) {
        Element* current = current_element();
        if (current == nullptr || current == table) {
            return;
        }
        if (is_table_structure_tag(current->tag_name())) {
            return;
        }
        m_element_stack.pop_back();
    }
}

bool TreeBuilder::try_recover_formatting_end_tag(const std::string_view tag_name) {
    if (!is_adoption_formatting_tag(tag_name)) {
        return false;
    }

    size_t matching_index = m_element_stack.size();
    for (size_t index = m_element_stack.size(); index > m_stack_floor; --index) {
        if (equals_ignore_case(m_element_stack[index - 1]->tag_name(), tag_name)) {
            matching_index = index - 1;
            break;
        }
    }

    if (matching_index == m_element_stack.size()) {
        return false;
    }
    if (matching_index + 1 == m_element_stack.size()) {
        return false;
    }

    for (size_t index = matching_index + 1; index < m_element_stack.size(); ++index) {
        if (!is_adoption_formatting_tag(m_element_stack[index]->tag_name())) {
            return false;
        }
    }

    std::vector<const Element*> reopen_chain;
    reopen_chain.reserve(m_element_stack.size() - matching_index - 1);
    for (size_t index = matching_index + 1; index < m_element_stack.size(); ++index) {
        reopen_chain.push_back(m_element_stack[index]);
    }

    m_element_stack.resize(matching_index);

    Node* parent = current_element() != nullptr
                       ? static_cast<Node*>(current_element())
                       : static_cast<Node*>(m_document.get());
    for (const Element* element : reopen_chain) {
        auto clone = clone_element_shallow(*element);
        auto* clone_ptr =
            const_cast<Element*>(insert_node(std::move(clone), parent)->as_element());
        push_element(clone_ptr);
        parent = clone_ptr;
    }

    return true;
}

Element* TreeBuilder::find_open_element(
    const std::string_view tag_name,
    const bool include_fragment_base) const noexcept {
    for (size_t index = m_element_stack.size(); index > 0; --index) {
        if (!include_fragment_base && index <= m_stack_floor) {
            break;
        }
        Element* element = m_element_stack[index - 1];
        if (equals_ignore_case(element->tag_name(), tag_name)) {
            return element;
        }
    }
    return nullptr;
}

Element* TreeBuilder::find_open_in_select_scope(const std::string_view tag_name) const noexcept {
    for (size_t index = m_element_stack.size(); index > 0; --index) {
        if (index <= m_stack_floor) {
            break;
        }
        Element* element = m_element_stack[index - 1];
        if (equals_ignore_case(element->tag_name(), tag_name)) {
            return element;
        }
        if (equals_ignore_case(element->tag_name(), "select")) {
            return equals_ignore_case(tag_name, "select") ? element : nullptr;
        }
    }
    return nullptr;
}

Element* TreeBuilder::find_open_table_section() const noexcept {
    for (size_t index = m_element_stack.size(); index > 0; --index) {
        Element* element = m_element_stack[index - 1];
        if (is_table_section_tag(element->tag_name())) {
            return element;
        }
        if (equals_ignore_case(element->tag_name(), "table")) {
            break;
        }
    }
    return nullptr;
}

Element* TreeBuilder::find_open_table_row() const noexcept {
    for (size_t index = m_element_stack.size(); index > 0; --index) {
        Element* element = m_element_stack[index - 1];
        if (equals_ignore_case(element->tag_name(), "tr")) {
            return element;
        }
        if (equals_ignore_case(element->tag_name(), "table")) {
            break;
        }
    }
    return nullptr;
}

Element* TreeBuilder::find_open_table_cell() const noexcept {
    for (size_t index = m_element_stack.size(); index > 0; --index) {
        Element* element = m_element_stack[index - 1];
        if (is_table_cell_tag(element->tag_name())) {
            return element;
        }
        if (equals_ignore_case(element->tag_name(), "table")) {
            break;
        }
    }
    return nullptr;
}

}  // namespace hps
