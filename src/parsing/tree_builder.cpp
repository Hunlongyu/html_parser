#include "hps/parsing/tree_builder.hpp"

#include "hps/core/comment_node.hpp"
#include "hps/core/doctype_node.hpp"
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

// 插入这些元素会向活动格式化列表压入 marker（其闭合时清理到该 marker）。
[[nodiscard]] bool is_marker_scope_tag(const std::string_view tag_name) noexcept {
    return hps::equals_ignore_case(tag_name, "td") || hps::equals_ignore_case(tag_name, "th") ||
           hps::equals_ignore_case(tag_name, "caption") || hps::equals_ignore_case(tag_name, "applet") ||
           hps::equals_ignore_case(tag_name, "marquee") || hps::equals_ignore_case(tag_name, "object") ||
           hps::equals_ignore_case(tag_name, "template");
}

// HTML5 外来内容里会「跳出」回 HTML 的起始标签集合（数组保持字典序以便二分）。
[[nodiscard]] bool is_foreign_breakout_tag(const std::string_view tag_name) noexcept {
    static constexpr std::array<std::string_view, 44> breakout_tags = {
        "b", "big", "blockquote", "body", "br", "center", "code", "dd", "div", "dl", "dt",
        "em", "embed", "h1", "h2", "h3", "h4", "h5", "h6", "head", "hr", "i", "img", "li",
        "listing", "menu", "meta", "nobr", "ol", "p", "pre", "ruby", "s", "small", "span",
        "strike", "strong", "sub", "sup", "table", "tt", "u", "ul", "var"};
    return std::ranges::binary_search(breakout_tags, tag_name);
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
                process_doctype(token);
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
    // HTML5：非片段文档总是产出 html/head/body 外壳——即便输入为空，或只有 head 内容
    // （如 <script> 后无 body 内容）也必须补出一个空 body。已存在时为幂等空操作。
    if (m_fragment_context == nullptr) {
        ensure_body_element();
    }

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
    // 外来内容（SVG/MathML）中的 breakout 起始标签：弹出外来元素回到 HTML 上下文，
    // 再按 HTML 规则继续处理该标签（如 <svg><g><p> 中的 <p> 应成为 HTML 段落）。
    if (current_insertion_namespace() != NamespaceKind::Html &&
        is_foreign_breakout_tag(token.name())) {
        while (m_element_stack.size() > m_stack_floor && current_element() != nullptr &&
               current_element()->namespace_kind() != NamespaceKind::Html) {
            m_element_stack.pop_back();
        }
    }

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

    // "in body"：游离的表格结构起始标签（caption/col/colgroup/tbody/td/tfoot/th/thead/tr，
    // 无打开的 table 时）按 HTML5 忽略。仅在 HTML 插入上下文生效（含 MathML/SVG 集成点内），
    // 故 <math><mo><tr> 的 <tr> 被忽略，而 <math><tr> 仍作为 math 命名空间元素保留。
    if (m_fragment_context == nullptr && current_insertion_namespace() == NamespaceKind::Html &&
        is_table_structure_tag(token.name()) && !equals_ignore_case(token.name(), "table") &&
        find_open_element("table", false) == nullptr) {
        parse_error(ErrorCode::InvalidNesting, "Stray table-structure start tag ignored in body", m_last_position);
        return;
    }

    if (equals_ignore_case(token.name(), "form") &&
        find_open_element("form", false) != nullptr) {
        parse_error(ErrorCode::InvalidNesting, "Unexpected nested <form>", m_last_position);
        return;
    }
    if (equals_ignore_case(token.name(), "a")) {
        // <a> 起始：若活动格式化列表（最后一个 marker 之后）已有 <a>，先跑 AAA 并清理之。
        Element* open_a = nullptr;
        for (size_t i = m_active_formatting.size(); i > 0; --i) {
            Element* entry = m_active_formatting[i - 1];
            if (entry == nullptr) {
                break;
            }
            if (equals_ignore_case(entry->tag_name(), "a")) {
                open_a = entry;
                break;
            }
        }
        if (open_a != nullptr) {
            parse_error(ErrorCode::InvalidNesting, "Unexpected nested <a>", m_last_position);
            static_cast<void>(run_adoption_agency("a"));
            remove_from_active_formatting(open_a);
            if (const auto it = std::ranges::find(m_element_stack, open_a); it != m_element_stack.end()) {
                m_element_stack.erase(it);
            }
        }
    }
    if (equals_ignore_case(token.name(), "nobr")) {
        // <nobr> 起始：先重建；若已有 nobr 在 scope 内，跑 AAA 后再重建。
        reconstruct_active_formatting_elements();
        if (Element* open_nobr = find_open_element("nobr", false);
            open_nobr != nullptr && has_element_in_scope(open_nobr)) {
            parse_error(ErrorCode::InvalidNesting, "Unexpected nested <nobr>", m_last_position);
            static_cast<void>(run_adoption_agency("nobr"));
            reconstruct_active_formatting_elements();
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

    // HTML5 in-body：插入“任意其它起始标签”前先重建活动格式化元素；
    // 表格结构标签（in-table 各模式）与 head 内容标签不在此列。
    if (!is_table_structure_tag(token.name()) && !is_head_content_tag(token.name())) {
        reconstruct_active_formatting_elements();
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
        if (is_formatting_element(token.name())) {
            push_active_formatting(element_ptr);
        } else if (is_marker_scope_tag(token.name())) {
            push_active_formatting_marker();
        }
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
    // 表格上下文里的 </body> / </html>：HTML5 的 in-table “anything else” 把它们委派给
    // in-body 处理，仅切换插入模式（after-body / after-after-body），并不弹出已打开的
    // 表格内容。对我们的简化构建器而言等价于“忽略”——若照常拆栈会毁掉 foster parenting
    // 的插入点（例如 <table><td>...</html>foo 中的 foo 应留在 <td> 内）。
    if (m_fragment_context == nullptr &&
        (equals_ignore_case(tag_name, "body") || equals_ignore_case(tag_name, "html")) &&
        find_open_element("table", false) != nullptr) {
        parse_error(ErrorCode::MismatchedTag,
                    "Ignoring </" + std::string(tag_name) + "> while a table is open", m_last_position);
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
    if (is_formatting_element(tag_name)) {
        if (run_adoption_agency(tag_name)) {
            return;
        }
        // run_adoption_agency 返回 false 表示活动格式化列表无此元素，按 “any other end tag” 继续。
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

    // body 文本插入前重建活动格式化元素（重新打开被隐式关闭的 <b>/<i>/… ）。
    reconstruct_active_formatting_elements();
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

void TreeBuilder::process_doctype(const Token& token) {
    // 仅在“初始”插入模式接受 DOCTYPE：尚无根元素、且非 fragment。
    // 其余位置（已有内容/根元素，或片段解析）按 HTML5 视为解析错误并忽略。
    if (m_fragment_context != nullptr || m_html_element != nullptr) {
        parse_error(ErrorCode::InvalidToken, "Unexpected DOCTYPE", m_last_position);
        return;
    }

    auto doctype = std::make_unique<DoctypeNode>(
        token.name(),
        token.doctype_public_id(),
        token.doctype_system_id(),
        token.doctype_has_identifiers());
    insert_node(std::move(doctype), m_document.get());
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
                static_cast<TextNode*>(last)->append_text(text);
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
        static_cast<TextNode*>(previous)->append_text(text);
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
        if (is_marker_scope_tag(element->tag_name())) {
            clear_active_formatting_to_last_marker();
        }
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

        // <p> 会被这些块级起始标签隐式关闭（HTML5「have a p element in button scope」组，
        // 含 h1–h6/pre/listing/form/plaintext/xmp/table/hr 等）。数组必须保持字典序以便二分。
        static constexpr std::array<std::string_view, 39> p_closers = {
            "address", "article", "aside", "blockquote", "center", "dd", "details", "dialog",
            "dir", "div", "dl", "dt", "fieldset", "figcaption", "figure", "footer", "form",
            "h1", "h2", "h3", "h4", "h5", "h6", "header", "hgroup", "hr", "listing", "main",
            "menu", "nav", "ol", "p", "plaintext", "pre", "section", "summary", "table", "ul", "xmp"};
        const bool closes_paragraph =
            current_tag == "p" && std::ranges::binary_search(p_closers, tag_name);
        const bool closes_list_item =
            (current_tag == "li" && tag_name == "li") ||
            ((current_tag == "dd" || current_tag == "dt") &&
             (tag_name == "dd" || tag_name == "dt"));
        const bool closes_button =
            equals_ignore_case(current_tag, "button") &&
            equals_ignore_case(tag_name, "button");
        // 一个 h1–h6 起始标签会关闭当前打开的 h1–h6（标题不可嵌套）。
        const bool closes_heading =
            is_heading_tag(current_tag) && is_heading_tag(tag_name);
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
            closes_paragraph || closes_list_item || closes_button || closes_heading ||
            closes_table_cell || closes_table_row || closes_table_section;

        if (should_pop_current) {
            m_element_stack.pop_back();
            if (is_marker_scope_tag(current_tag)) {
                clear_active_formatting_to_last_marker();
            }
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

bool TreeBuilder::is_heading_tag(const std::string_view tag_name) noexcept {
    static constexpr auto heading_tags = sorted_string_view_array(
        std::array<std::string_view, 6>{"h1", "h2", "h3", "h4", "h5", "h6"});
    return std::ranges::binary_search(heading_tags, tag_name);
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

NamespaceKind TreeBuilder::current_insertion_namespace() const noexcept {
    const auto* current = current_element();
    if (current == nullptr) {
        return NamespaceKind::Html;
    }
    const NamespaceKind    ns  = current->namespace_kind();
    const std::string_view tag = current->tag_name();
    if (ns == NamespaceKind::Svg) {
        // SVG HTML 集成点：foreignObject / desc / title 的内容按 HTML 解析。
        if (equals_ignore_case(tag, "foreignobject") || equals_ignore_case(tag, "desc") ||
            equals_ignore_case(tag, "title")) {
            return NamespaceKind::Html;
        }
    } else if (ns == NamespaceKind::MathML) {
        // MathML 文本集成点：mi/mo/mn/ms/mtext 的内容按 HTML 解析。
        if (equals_ignore_case(tag, "mi") || equals_ignore_case(tag, "mo") ||
            equals_ignore_case(tag, "mn") || equals_ignore_case(tag, "ms") ||
            equals_ignore_case(tag, "mtext")) {
            return NamespaceKind::Html;
        }
    }
    return ns;
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

// ==================== 活动格式化元素列表 + 领养机构算法 ====================

bool TreeBuilder::is_formatting_element(const std::string_view tag_name) noexcept {
    static constexpr std::array<std::string_view, 14> formatting = {
        "a", "b", "big", "code", "em", "font", "i", "nobr", "s", "small", "strike", "strong", "tt", "u"};
    return std::ranges::find(formatting, tag_name) != formatting.end();
}

bool TreeBuilder::is_special_element(const Element& element) noexcept {
    if (element.namespace_kind() != NamespaceKind::Html) {
        const std::string_view tag = element.tag_name();
        return equals_ignore_case(tag, "foreignobject") || equals_ignore_case(tag, "desc") ||
               equals_ignore_case(tag, "title") || equals_ignore_case(tag, "mi") ||
               equals_ignore_case(tag, "mo") || equals_ignore_case(tag, "mn") ||
               equals_ignore_case(tag, "ms") || equals_ignore_case(tag, "mtext") ||
               equals_ignore_case(tag, "annotation-xml");
    }
    static constexpr std::array<std::string_view, 83> special = {
        "address", "applet", "area", "article", "aside", "base", "basefont", "bgsound",
        "blockquote", "body", "br", "button", "caption", "center", "col", "colgroup", "dd",
        "details", "dir", "div", "dl", "dt", "embed", "fieldset", "figcaption", "figure",
        "footer", "form", "frame", "frameset", "h1", "h2", "h3", "h4", "h5", "h6", "head",
        "header", "hgroup", "hr", "html", "iframe", "img", "input", "keygen", "li", "link",
        "listing", "main", "marquee", "menu", "meta", "nav", "noembed", "noframes", "noscript",
        "object", "ol", "p", "param", "plaintext", "pre", "script", "section", "select",
        "source", "style", "summary", "table", "tbody", "td", "template", "textarea", "tfoot",
        "th", "thead", "title", "tr", "track", "ul", "wbr", "xmp"};
    return std::ranges::find(special, element.tag_name()) != special.end();
}

bool TreeBuilder::same_formatting_element(const Element& a, const Element& b) noexcept {
    if (a.namespace_kind() != b.namespace_kind() || !equals_ignore_case(a.tag_name(), b.tag_name())) {
        return false;
    }
    const auto& aa = a.attributes();
    const auto& ba = b.attributes();
    if (aa.size() != ba.size()) {
        return false;
    }
    for (const auto& attr : aa) {
        const bool found = std::ranges::any_of(ba, [&attr](const auto& other) {
            return other.name() == attr.name() && other.value() == attr.value();
        });
        if (!found) {
            return false;
        }
    }
    return true;
}

bool TreeBuilder::is_in_active_formatting(const Element* element) const noexcept {
    return element != nullptr &&
           std::ranges::find(m_active_formatting, element) != m_active_formatting.end();
}

void TreeBuilder::remove_from_active_formatting(const Element* element) {
    const auto it = std::ranges::find(m_active_formatting, element);
    if (it != m_active_formatting.end()) {
        m_active_formatting.erase(it);
    }
}

void TreeBuilder::push_active_formatting_marker() {
    m_active_formatting.push_back(nullptr);
}

void TreeBuilder::clear_active_formatting_to_last_marker() {
    while (!m_active_formatting.empty()) {
        Element* entry = m_active_formatting.back();
        m_active_formatting.pop_back();
        if (entry == nullptr) {
            break;
        }
    }
}

void TreeBuilder::push_active_formatting(Element* element) {
    // Noah's Ark：最后一个 marker 之后，若已有 3 个“同名同命名空间同属性”的条目，移除最早者。
    int    count    = 0;
    size_t earliest = m_active_formatting.size();
    for (size_t i = m_active_formatting.size(); i > 0; --i) {
        Element* entry = m_active_formatting[i - 1];
        if (entry == nullptr) {
            break;  // marker
        }
        if (same_formatting_element(*entry, *element)) {
            ++count;
            earliest = i - 1;
        }
    }
    if (count >= 3 && earliest < m_active_formatting.size()) {
        m_active_formatting.erase(m_active_formatting.begin() + static_cast<std::ptrdiff_t>(earliest));
    }
    m_active_formatting.push_back(element);
}

Element* TreeBuilder::insert_html_element_at_current(std::unique_ptr<Element> element) {
    Node* parent = m_element_stack.empty() ? static_cast<Node*>(m_document.get())
                                           : static_cast<Node*>(current_element());
    return const_cast<Element*>(insert_node(std::move(element), parent)->as_element());
}

void TreeBuilder::reconstruct_active_formatting_elements() {
    if (m_active_formatting.empty()) {
        return;
    }
    Element* last = m_active_formatting.back();
    if (last == nullptr || is_on_stack(last)) {
        return;  // 末项是 marker 或仍在开放栈中：无需重建
    }

    size_t index = m_active_formatting.size() - 1;
    while (index > 0) {
        Element* entry = m_active_formatting[index - 1];
        if (entry == nullptr || is_on_stack(entry)) {
            break;
        }
        --index;
    }

    for (; index < m_active_formatting.size(); ++index) {
        Element* entry = m_active_formatting[index];
        auto*    clone = insert_html_element_at_current(clone_element_shallow(*entry));
        push_element(clone);
        m_active_formatting[index] = clone;
    }
}

bool TreeBuilder::has_element_in_scope(const Element* target) const noexcept {
    for (size_t i = m_element_stack.size(); i > 0; --i) {
        Element* element = m_element_stack[i - 1];
        if (element == target) {
            return true;
        }
        const std::string_view tag = element->tag_name();
        if (element->namespace_kind() == NamespaceKind::Html) {
            if (equals_ignore_case(tag, "applet") || equals_ignore_case(tag, "caption") ||
                equals_ignore_case(tag, "html") || equals_ignore_case(tag, "table") ||
                equals_ignore_case(tag, "td") || equals_ignore_case(tag, "th") ||
                equals_ignore_case(tag, "marquee") || equals_ignore_case(tag, "object") ||
                equals_ignore_case(tag, "template")) {
                return false;
            }
        } else {
            // MathML 文本集成点 / SVG HTML 集成点也是 scope 边界。
            if (is_special_element(*element)) {
                return false;
            }
        }
    }
    return false;
}

namespace {
// 把 node 从当前父节点（应为元素）摘下，挂到 new_parent（before 非空则插于其前）。
void move_node_into(Node* node, Element* new_parent, const Node* before) {
    if (node == nullptr || new_parent == nullptr || node == new_parent) {
        return;  // 防御：绝不把节点挂到自身之下（会形成环/二次释放）。
    }
    auto* old_parent = node->parent() ? const_cast<Element*>(node->parent()->as_element()) : nullptr;
    if (old_parent == nullptr) {
        return;
    }
    std::unique_ptr<Node> owned = old_parent->remove_child(node);
    if (!owned) {
        return;
    }
    if (before != nullptr) {
        new_parent->insert_child_before(std::move(owned), before);
    } else {
        new_parent->add_child(std::move(owned));
    }
}
}  // namespace

bool TreeBuilder::run_adoption_agency(const std::string_view subject) {
    // 1. 当前节点即 subject 且不在活动格式化列表：直接弹出。
    if (Element* current = current_element();
        current != nullptr && current->namespace_kind() == NamespaceKind::Html &&
        equals_ignore_case(current->tag_name(), subject) && !is_in_active_formatting(current)) {
        m_element_stack.pop_back();
        return true;
    }

    for (int outer = 0; outer < 8; ++outer) {
        // a. 活动格式化列表中最后一个 marker 之后、名字为 subject 的条目。
        Element* formatting_element = nullptr;
        size_t   fe_afe_index       = 0;
        for (size_t i = m_active_formatting.size(); i > 0; --i) {
            Element* entry = m_active_formatting[i - 1];
            if (entry == nullptr) {
                break;
            }
            if (equals_ignore_case(entry->tag_name(), subject)) {
                formatting_element = entry;
                fe_afe_index       = i - 1;
                break;
            }
        }
        if (formatting_element == nullptr) {
            return false;  // 交给 “any other end tag” 处理
        }

        // b. 不在开放元素栈：从活动格式化列表移除并返回。
        if (!is_on_stack(formatting_element)) {
            parse_error(ErrorCode::MismatchedTag, "Adoption agency: formatting element not open");
            remove_from_active_formatting(formatting_element);
            return true;
        }
        // c. 在栈但不在 scope：什么也不做。
        if (!has_element_in_scope(formatting_element)) {
            parse_error(ErrorCode::MismatchedTag, "Adoption agency: formatting element not in scope");
            return true;
        }

        // e. furthest block：栈中位于 formatting_element 之下、最靠近它的 special 元素。
        size_t fe_stack_index = 0;
        for (size_t i = 0; i < m_element_stack.size(); ++i) {
            if (m_element_stack[i] == formatting_element) {
                fe_stack_index = i;
                break;
            }
        }
        Element* furthest_block = nullptr;
        size_t   fb_stack_index = 0;
        for (size_t i = fe_stack_index + 1; i < m_element_stack.size(); ++i) {
            if (is_special_element(*m_element_stack[i])) {
                furthest_block = m_element_stack[i];
                fb_stack_index = i;
                break;
            }
        }

        // f. 无 furthest block：弹栈至（含）formatting_element，移除活动格式化条目。
        if (furthest_block == nullptr) {
            while (!m_element_stack.empty() && current_element() != formatting_element) {
                m_element_stack.pop_back();
            }
            if (!m_element_stack.empty()) {
                m_element_stack.pop_back();  // 弹出 formatting_element 自身
            }
            remove_from_active_formatting(formatting_element);
            return true;
        }

        // g. common ancestor：栈中 formatting_element 紧靠栈底一侧的元素。
        if (fe_stack_index == 0) {
            return true;  // 理论上 FE 之下总有 html/上下文元素；防御性返回。
        }
        Element* common_ancestor = m_element_stack[fe_stack_index - 1];

        // h. bookmark：formatting_element 在活动格式化列表中的位置。
        size_t bookmark = fe_afe_index;

        // i. 内循环：以下标跟踪，从 furthest block 向 formatting_element 方向（栈下方）遍历，
        //    沿途克隆活动格式化元素、丢弃非活动格式化元素，并把子树逐层下挂。
        Element* node       = furthest_block;
        Element* last_node  = furthest_block;
        size_t   node_index = fb_stack_index;
        for (int inner = 1;; ++inner) {
            if (node_index == 0) {
                break;
            }
            --node_index;
            node = m_element_stack[node_index];

            if (node == formatting_element) {
                break;
            }
            if (inner > 3 && is_in_active_formatting(node)) {
                remove_from_active_formatting(node);
            }
            if (!is_in_active_formatting(node)) {
                // 非活动格式化元素：从开放栈摘除（仅影响其上方元素，不影响继续向下遍历）。
                m_element_stack.erase(m_element_stack.begin() + static_cast<std::ptrdiff_t>(node_index));
                continue;
            }

            // 克隆 node，在活动格式化列表与开放栈中原位替换。
            auto* clone = const_cast<Element*>(
                common_ancestor->add_child(clone_element_shallow(*node))->as_element());
            if (const auto afe_it = std::ranges::find(m_active_formatting, node);
                afe_it != m_active_formatting.end()) {
                *afe_it = clone;
            }
            m_element_stack[node_index] = clone;
            node = clone;

            if (last_node == furthest_block) {
                bookmark = static_cast<size_t>(
                               std::ranges::find(m_active_formatting, node) - m_active_formatting.begin()) +
                           1;
            }

            // 把 last_node 挂到 node 之下。
            move_node_into(last_node, node, nullptr);
            last_node = node;
        }

        // 9. 把 last_node 放到 common ancestor 的“合适位置”（表格上下文需 foster）。
        if (should_foster_parent_element(common_ancestor->tag_name()) ||
            equals_ignore_case(common_ancestor->tag_name(), "table") ||
            is_table_section_tag(common_ancestor->tag_name()) ||
            equals_ignore_case(common_ancestor->tag_name(), "tr")) {
            const auto [parent, before] = foster_parent_insertion_point();
            if (auto* parent_el = parent ? const_cast<Element*>(parent->as_element()) : nullptr) {
                move_node_into(last_node, parent_el, before);
            } else {
                move_node_into(last_node, common_ancestor, nullptr);
            }
        } else {
            move_node_into(last_node, common_ancestor, nullptr);
        }

        // 10. 克隆 formatting_element，把 furthest block 的所有子节点搬到克隆下，再挂回 furthest block。
        auto* fe_clone = const_cast<Element*>(
            furthest_block->add_child(clone_element_shallow(*formatting_element))->as_element());
        for (auto* child = const_cast<Node*>(furthest_block->first_child()); child != nullptr;) {
            auto* next = const_cast<Node*>(child->next_sibling());
            if (child != fe_clone) {
                move_node_into(child, fe_clone, nullptr);
            }
            child = next;
        }
        // 上面的循环已把 fe_clone 之前的子节点搬走；确保 fe_clone 在 furthest block 末尾。
        // （add_child 已追加在末尾，搬运时跳过它本身。）

        // 11-13. 从活动格式化列表与开放栈中移除 formatting_element，于 bookmark 处插入 fe_clone，
        //         并把 fe_clone 放到 furthest block 紧上方（更靠栈顶一侧）。
        remove_from_active_formatting(formatting_element);
        // FE 原位于 fe_afe_index；移除后其右侧的 bookmark 需左移一位。
        if (bookmark > fe_afe_index) {
            --bookmark;
        }
        if (bookmark > m_active_formatting.size()) {
            bookmark = m_active_formatting.size();
        }
        m_active_formatting.insert(
            m_active_formatting.begin() + static_cast<std::ptrdiff_t>(bookmark), fe_clone);

        if (const auto fe_stk = std::ranges::find(m_element_stack, formatting_element);
            fe_stk != m_element_stack.end()) {
            m_element_stack.erase(fe_stk);
        }
        if (const auto fb_stk = std::ranges::find(m_element_stack, furthest_block);
            fb_stk != m_element_stack.end()) {
            m_element_stack.insert(fb_stk + 1, fe_clone);
        }
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
