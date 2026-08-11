#include "hps/parsing/tree_builder.hpp"

#include "hps/core/comment_node.hpp"
#include "hps/core/doctype_node.hpp"
#include "hps/core/document.hpp"
#include "hps/core/tag_table.hpp"
#include "hps/core/text_node.hpp"
#include "hps/parsing/token.hpp"
#include "hps/utils/string_utils.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>
#include <string_view>
#include <unordered_map>

namespace hps {

namespace {

// SVG 属性大小写修正（HTML 词法器全小写 → 还原 camelCase）。HTML5「adjust SVG attributes」表。
[[nodiscard]] std::string_view adjust_svg_attribute_name(const std::string_view name) {
    static const std::unordered_map<std::string_view, std::string_view> table = {{"attributename", "attributeName"},
                                                                                 {"attributetype", "attributeType"},
                                                                                 {"basefrequency", "baseFrequency"},
                                                                                 {"baseprofile", "baseProfile"},
                                                                                 {"calcmode", "calcMode"},
                                                                                 {"clippathunits", "clipPathUnits"},
                                                                                 {"diffuseconstant", "diffuseConstant"},
                                                                                 {"edgemode", "edgeMode"},
                                                                                 {"filterunits", "filterUnits"},
                                                                                 {"glyphref", "glyphRef"},
                                                                                 {"gradienttransform", "gradientTransform"},
                                                                                 {"gradientunits", "gradientUnits"},
                                                                                 {"kernelmatrix", "kernelMatrix"},
                                                                                 {"kernelunitlength", "kernelUnitLength"},
                                                                                 {"keypoints", "keyPoints"},
                                                                                 {"keysplines", "keySplines"},
                                                                                 {"keytimes", "keyTimes"},
                                                                                 {"lengthadjust", "lengthAdjust"},
                                                                                 {"limitingconeangle", "limitingConeAngle"},
                                                                                 {"markerheight", "markerHeight"},
                                                                                 {"markerunits", "markerUnits"},
                                                                                 {"markerwidth", "markerWidth"},
                                                                                 {"maskcontentunits", "maskContentUnits"},
                                                                                 {"maskunits", "maskUnits"},
                                                                                 {"numoctaves", "numOctaves"},
                                                                                 {"pathlength", "pathLength"},
                                                                                 {"patterncontentunits", "patternContentUnits"},
                                                                                 {"patterntransform", "patternTransform"},
                                                                                 {"patternunits", "patternUnits"},
                                                                                 {"pointsatx", "pointsAtX"},
                                                                                 {"pointsaty", "pointsAtY"},
                                                                                 {"pointsatz", "pointsAtZ"},
                                                                                 {"preservealpha", "preserveAlpha"},
                                                                                 {"preserveaspectratio", "preserveAspectRatio"},
                                                                                 {"primitiveunits", "primitiveUnits"},
                                                                                 {"refx", "refX"},
                                                                                 {"refy", "refY"},
                                                                                 {"repeatcount", "repeatCount"},
                                                                                 {"repeatdur", "repeatDur"},
                                                                                 {"requiredextensions", "requiredExtensions"},
                                                                                 {"requiredfeatures", "requiredFeatures"},
                                                                                 {"specularconstant", "specularConstant"},
                                                                                 {"specularexponent", "specularExponent"},
                                                                                 {"spreadmethod", "spreadMethod"},
                                                                                 {"startoffset", "startOffset"},
                                                                                 {"stddeviation", "stdDeviation"},
                                                                                 {"stitchtiles", "stitchTiles"},
                                                                                 {"surfacescale", "surfaceScale"},
                                                                                 {"systemlanguage", "systemLanguage"},
                                                                                 {"tablevalues", "tableValues"},
                                                                                 {"targetx", "targetX"},
                                                                                 {"targety", "targetY"},
                                                                                 {"textlength", "textLength"},
                                                                                 {"viewbox", "viewBox"},
                                                                                 {"viewtarget", "viewTarget"},
                                                                                 {"xchannelselector", "xChannelSelector"},
                                                                                 {"ychannelselector", "yChannelSelector"},
                                                                                 {"zoomandpan", "zoomAndPan"}};
    const auto                                                          it    = table.find(name);
    return it != table.end() ? it->second : name;
}

// 外来元素属性名修正（SVG 大小写表；MathML definitionurl → definitionURL）。
[[nodiscard]] std::string_view adjust_foreign_attribute_name(const std::string_view name, const NamespaceKind ns) {
    if (ns == NamespaceKind::Svg) {
        return adjust_svg_attribute_name(name);
    }
    if (ns == NamespaceKind::MathML && name == "definitionurl") {
        return "definitionURL";
    }
    return name;
}

}  // namespace

TreeBuilder::TreeBuilder(const std::shared_ptr<Document>& document, const Options& options)
    : m_document(document),
      m_options(options) {
    assert(m_document != nullptr);
    m_element_stack.reserve(32);
    m_ignored_element_stack.reserve(8);
}

TreeBuilder::TreeBuilder(const std::shared_ptr<Document>& document, const Options& options, Element* fragment_context)
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
    // 例外：frameset 文档没有 body。
    if (m_fragment_context == nullptr && !m_is_frameset_document) {
        ensure_body_element();
    }

    while (m_element_stack.size() > m_stack_floor) {
        const auto element = m_element_stack.back();
        m_element_stack.pop_back();
        if (!tag::is_optional_end(element->tag())) {
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
    const std::string_view name = token.name();
    const Tag              tg   = tag::from_name_ci(name);  // 整数化一次，热路径全程整数分发

    apply_foreign_content_breakout(tg);

    // <frameset> 起始标签（根或嵌套）由专门方法处理。
    if (m_fragment_context == nullptr && tg == tag::kFrameset) {
        process_frameset_start_tag(token);
        return;
    }

    // frameset 文档：in-frameset 仅允许 frame/noframes；after-frameset（根 frameset 已关闭）
    // 仅允许 noframes；其余起始标签忽略，且不走文档外壳逻辑（frameset 文档无 body）。
    if (m_fragment_context == nullptr && m_is_frameset_document) {
        const bool frameset_open = find_open_element(tag::kFrameset, false) != nullptr;
        const bool allowed       = tg == tag::kNoframes || (frameset_open && tg == tag::kFrame);
        if (!allowed) {
            parse_error(ErrorCode::InvalidNesting, "Start tag ignored in frameset document", m_last_position);
            return;
        }
    } else if (m_fragment_context == nullptr && find_open_element(tag::kTemplate, false) == nullptr) {
        // template 内容隔离：有打开的 <template> 时跳过文档外壳管理，内容直接落入 template 子树。
        if (tg == tag::kHtml) {
            process_html_start_tag(token);
            return;
        }
        if (tg == tag::kHead) {
            process_head_start_tag(token);
            return;
        }
        if (tg == tag::kBody) {
            process_body_start_tag(token);
            return;
        }

        if (tag::is_head_content(tg) && m_body_element == nullptr) {
            ensure_head_element();
        } else {
            if (current_element() == m_head_element && !m_head_closed) {
                close_head_element_if_open();
            }
            ensure_body_element();
        }
    }

    // "in body"：游离的表格结构起始标签（caption/col/colgroup/tbody/td/tfoot/th/thead/tr，
    // 无打开的 table 时）按 HTML5 忽略。仅在 HTML 插入上下文生效（含 MathML/SVG 集成点内），
    // 故 <math><mo><tr> 的 <tr> 被忽略，而 <math><tr> 仍作为 math 命名空间元素保留。
    if (m_fragment_context == nullptr && current_insertion_namespace() == NamespaceKind::Html && tag::is_table_structure(tg) && tg != tag::kTable && find_open_element(tag::kTable, false) == nullptr) {
        parse_error(ErrorCode::InvalidNesting, "Stray table-structure start tag ignored in body", m_last_position);
        return;
    }

    if (tg == tag::kForm && find_open_element(tag::kForm, false) != nullptr) {
        parse_error(ErrorCode::InvalidNesting, "Unexpected nested <form>", m_last_position);
        return;
    }
    if (tg == tag::kA) {
        handle_a_start_tag();
    }
    if (tg == tag::kNobr) {
        handle_nobr_start_tag();
    }

    if (tg != tag::kCol) {
        close_colgroup_for_non_col_token();
    }

    const bool foster_parent_element = should_foster_parent_element(tg);
    check_implicit_close(tg);
    if (!foster_parent_element) {
        prepare_table_context_for_start_tag(tg);
    }
    prepare_select_context_for_start_tag(tg);

    const size_t content_depth = static_cast<size_t>(std::ranges::count_if(m_element_stack, [this](const Element* element) { return element != m_html_element && element != m_head_element && element != m_body_element; }));
    const size_t next_depth    = content_depth + 1;
    if (next_depth > m_options.max_depth) {
        parse_error(ErrorCode::TooDeep, "Nesting depth limit exceeded at <" + std::string(token.name()) + ">", m_last_position);
        if (!m_options.is_void_element(token.name()) && token.type() != TokenType::CLOSE_SELF) {
            m_ignored_element_stack.emplace_back(token.name());
        }
        return;
    }

    if (tg == tag::kBr) {
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
    if (!tag::is_table_structure(tg) && !tag::is_head_content(tg)) {
        reconstruct_active_formatting_elements();
    }

    Element* element_ptr = create_element(token, namespace_for_start_tag(tg));
    if (foster_parent_element) {
        const auto [parent, before] = foster_parent_insertion_point();
        insert_node_before(element_ptr, parent, before);
    } else {
        insert_element(element_ptr);
    }

    if (!m_options.is_void_element(name) && token.type() != TokenType::CLOSE_SELF) {
        push_element(element_ptr);
        if (tag::is_formatting(tg)) {
            push_active_formatting(element_ptr);
        } else if (tag::is_marker_scope(tg)) {
            push_active_formatting_marker();
        }
    }
}

void TreeBuilder::process_html_start_tag(const Token& token) {
    if (!m_html_element) {
        Element* html_element = create_element(token);
        m_html_element        = const_cast<Element*>(insert_node(html_element, m_document.get())->as_element());
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
        Element* head_element = create_element(token);
        m_head_element        = const_cast<Element*>(insert_node(head_element, m_html_element)->as_element());
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
        Element* body_element = create_element(token);
        m_body_element        = const_cast<Element*>(insert_node(body_element, m_html_element)->as_element());
    } else {
        merge_token_attributes(*m_body_element, token);
    }

    if (token.type() != TokenType::CLOSE_SELF) {
        push_if_absent(m_body_element);
    }
}

void TreeBuilder::process_frameset_start_tag(const Token& token) {
    ensure_html_element();

    // 嵌套 frameset：已在 frameset 内 → 原位插入。
    if (find_open_element(tag::kFrameset, false) != nullptr) {
        auto* frameset = const_cast<Element*>(insert_node(create_element(token), current_element())->as_element());
        push_element(frameset);
        return;
    }

    // after-frameset：根 frameset 已关闭后再来 <frameset> → 忽略。
    if (m_is_frameset_document) {
        parse_error(ErrorCode::InvalidNesting, "Unexpected <frameset> after frameset", m_last_position);
        return;
    }

    // 根 frameset：仅当 body 尚为空时接受（近似 HTML5 的 frameset-ok 标志）；否则忽略。
    if (m_body_element != nullptr && m_body_element->has_children()) {
        parse_error(ErrorCode::InvalidNesting, "Unexpected <frameset>", m_last_position);
        return;
    }

    if (!m_head_element) {
        ensure_head_element();
    }
    close_head_element_if_open();

    // 移除已创建的空 body（frameset 文档无 body）。
    if (m_body_element != nullptr) {
        if (is_on_stack(m_body_element)) {
            close_elements_until(tag::kBody, false);
        }
        m_html_element->remove_child(m_body_element);
        m_body_element = nullptr;
    }

    auto* frameset = const_cast<Element*>(insert_node(create_element(token), m_html_element)->as_element());
    push_element(frameset);
    m_is_frameset_document = true;
}

// process_start_tag 的可拆分子步骤（与 tree_builder.hpp 声明顺序一致）。
void TreeBuilder::apply_foreign_content_breakout(const Tag tag) {
    // 外来内容（SVG/MathML）中的 breakout 起始标签：弹出外来元素回到 HTML 上下文，
    // 再按 HTML 规则继续处理该标签（如 <svg><g><p> 中的 <p> 应成为 HTML 段落）。
    if (current_insertion_namespace() == NamespaceKind::Html || !tag::is_foreign_breakout(tag)) {
        return;
    }
    while (m_element_stack.size() > m_stack_floor && current_element() != nullptr && current_element()->namespace_kind() != NamespaceKind::Html) {
        m_element_stack.pop_back();
    }
}

void TreeBuilder::handle_a_start_tag() {
    // <a> 起始：若活动格式化列表（最后一个 marker 之后）已有 <a>，先跑 AAA 并清理之。
    Element* open_a = nullptr;
    for (size_t i = m_active_formatting.size(); i > 0; --i) {
        Element* entry = m_active_formatting[i - 1];
        if (entry == nullptr) {
            break;
        }
        if (entry->tag() == tag::kA) {
            open_a = entry;
            break;
        }
    }
    if (open_a == nullptr) {
        return;
    }
    parse_error(ErrorCode::InvalidNesting, "Unexpected nested <a>", m_last_position);
    static_cast<void>(run_adoption_agency(tag::kA));
    remove_from_active_formatting(open_a);
    if (const auto it = std::ranges::find(m_element_stack, open_a); it != m_element_stack.end()) {
        m_element_stack.erase(it);
    }
}

void TreeBuilder::handle_nobr_start_tag() {
    // <nobr> 起始：先重建；若已有 nobr 在 scope 内，跑 AAA 后再重建。
    reconstruct_active_formatting_elements();
    if (Element* open_nobr = find_open_element(tag::kNobr, false); open_nobr != nullptr && has_element_in_scope(open_nobr)) {
        parse_error(ErrorCode::InvalidNesting, "Unexpected nested <nobr>", m_last_position);
        static_cast<void>(run_adoption_agency(tag::kNobr));
        reconstruct_active_formatting_elements();
    }
}

void TreeBuilder::process_end_tag(const Token& token) {
    const std::string_view tag_name = token.name();
    const Tag              tg       = tag::from_name_ci(tag_name);

    if (m_fragment_context == nullptr && tg == tag::kHead) {
        close_head_element_if_open();
        return;
    }
    if (m_fragment_context == nullptr && tg == tag::kFrameset) {
        if (find_open_element(tag::kFrameset, false) != nullptr) {
            close_elements_until(tag::kFrameset, false);  // 弹出当前 frameset（树中保留）
        } else {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: frameset", m_last_position);
        }
        return;
    }
    // 表格上下文里的 </body> / </html>：HTML5 的 in-table “anything else” 把它们委派给
    // in-body 处理，仅切换插入模式（after-body / after-after-body），并不弹出已打开的
    // 表格内容。对我们的简化构建器而言等价于“忽略”——若照常拆栈会毁掉 foster parenting
    // 的插入点（例如 <table><td>...</html>foo 中的 foo 应留在 <td> 内）。
    if (m_fragment_context == nullptr && (tg == tag::kBody || tg == tag::kHtml) && find_open_element(tag::kTable, false) != nullptr) {
        parse_error(ErrorCode::MismatchedTag, "Ignoring </" + std::string(tag_name) + "> while a table is open", m_last_position);
        return;
    }
    if (m_fragment_context == nullptr && tg == tag::kBody) {
        close_head_element_if_open();
        if (!m_body_element) {
            return;
        }
        if (is_on_stack(m_body_element)) {
            close_elements_until(tag::kBody, false);
        }
        return;
    }
    if (m_fragment_context == nullptr && tg == tag::kHtml) {
        close_head_element_if_open();
        if (m_body_element && is_on_stack(m_body_element)) {
            close_elements_until(tag::kBody, false);
        }
        if (m_html_element && is_on_stack(m_html_element)) {
            close_elements_until(tag::kHtml, false);
        }
        return;
    }

    if (handle_table_end_tag(tg)) {
        return;
    }
    if (handle_select_end_tag(tg)) {
        return;
    }

    if (m_options.is_void_element(tag_name)) {
        return;
    }

    if (m_element_stack.empty()) {
        parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: " + std::string(tag_name));
        return;
    }
    if (tag::is_formatting(tg)) {
        if (run_adoption_agency(tg)) {
            return;
        }
        // run_adoption_agency 返回 false 表示活动格式化列表无此元素，按 “any other end tag” 继续。
    }

    // </template>：关闭最内层 template 本身。
    if (tg == tag::kTemplate) {
        if (find_open_element(tag::kTemplate, false) != nullptr) {
            close_elements_until(tag::kTemplate, false);
        } else {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: template");
        }
        return;
    }

    // 其它结束标签只在最内层 template 的内容范围内匹配（模板隔离：</div> 跨不出 template）。
    // 已知标签按整数比较；自定义标签（tg==Unknown，无法靠整数区分彼此）回退按名比较。
    size_t scope_floor = m_stack_floor;
    for (size_t i = m_element_stack.size(); i > 0; --i) {
        if (m_element_stack[i - 1]->tag() == tag::kTemplate) {
            scope_floor = i;
            break;
        }
    }
    const auto matches        = [&](const Element* el) { return tg != Tag::Unknown ? el->tag() == tg : equals_ignore_case(el->tag_name(), tag_name); };
    bool       found_in_scope = false;
    for (size_t i = m_element_stack.size(); i > scope_floor; --i) {
        if (matches(m_element_stack[i - 1])) {
            found_in_scope = true;
            break;
        }
    }
    if (found_in_scope) {
        if (tg != Tag::Unknown) {
            close_elements_until(tg);
        } else {
            close_elements_until_by_name(tag_name);
        }
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
    if (m_fragment_context == nullptr && (current_element() == nullptr || current_element() == m_html_element)) {
        ensure_body_element();
    }

    // 无变换（Raw + 不解码实体 + Preserve 空白）→ 直接用源码视图,零拷贝（intern 会识别
    // 为源码子串而不复制,文本节点零分配持有）。其余情形才落到 buffer 做变换。
    const bool needs_transform = m_options.text_processing_mode == TextProcessingMode::Decode || m_options.decode_entities || m_options.whitespace_mode != WhitespaceMode::Preserve;

    std::string      buffer;   // 仅在需变换时使用
    std::string_view content;  // 最终要插入的内容（源码视图 / buffer 视图）
    if (!needs_transform) {
        content = text;
    } else {
        std::string processed_text(text);
        if (m_options.text_processing_mode == TextProcessingMode::Decode && !token.entities_decoded()) {
            processed_text = decode_html_entities(processed_text);
        }
        if (m_options.decode_entities && m_options.text_processing_mode == TextProcessingMode::Raw && !token.entities_decoded()) {
            processed_text = decode_html_entities(processed_text);
        }
        switch (m_options.whitespace_mode) {
            case WhitespaceMode::Preserve:
                buffer = std::move(processed_text);
                break;
            case WhitespaceMode::Normalize:
                buffer = normalize_whitespace(processed_text);
                break;
            case WhitespaceMode::Trim:
                buffer = std::string(trim_whitespace(processed_text));
                break;
            case WhitespaceMode::Remove:
                return;
        }
        if (buffer.empty()) {
            return;
        }
        content = buffer;
    }

    if (should_foster_parent_text() && !is_all_whitespace(content)) {
        const auto [parent, before] = foster_parent_insertion_point();
        insert_text_before(content, parent, before);
        return;
    }

    // body 文本插入前重建活动格式化元素（重新打开被隐式关闭的 <b>/<i>/… ）。
    reconstruct_active_formatting_elements();
    insert_text(content);
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

    DoctypeNode* doctype = m_document->create_doctype(token.name(), token.doctype_public_id(), token.doctype_system_id(), token.doctype_has_identifiers());
    insert_node(doctype, m_document.get());
}

Element* TreeBuilder::create_element(const Token& token) {
    return create_element(token, NamespaceKind::Html);
}

Element* TreeBuilder::create_element(const Token& token, const NamespaceKind namespace_kind) {
    Element* element = m_document->create_element(token.name(), namespace_kind);
    merge_token_attributes(*element, token);
    return element;
}

Element* TreeBuilder::clone_element_shallow(const Element& source) const {
    Element* clone = m_document->create_element(source.tag_name(), source.namespace_kind());
    for (const auto& attribute : source.attributes()) {
        clone->add_attribute(attribute.name(), attribute.value(), attribute.has_value());
    }
    return clone;
}

void TreeBuilder::merge_token_attributes(Element& element, const Token& token) const {
    const NamespaceKind ns = element.namespace_kind();
    for (const auto& attr : token.attrs()) {
        const std::string_view name = ns == NamespaceKind::Html ? std::string_view(attr.name) : adjust_foreign_attribute_name(attr.name, ns);
        if (m_options.decode_entities && attr.has_value) {
            const std::string decoded = decode_html_attribute_entities(attr.value);
            element.add_attribute(name, decoded, true);
        } else {
            element.add_attribute(name, attr.value, attr.has_value);
        }
    }
}

void TreeBuilder::insert_element(Element* element) const {
    if (m_element_stack.empty()) {
        m_document->add_child(element);
    } else {
        current_element()->add_child(element);
    }
}

Node* TreeBuilder::insert_node(Node* child, Node* parent) const {
    if (child == nullptr) {
        return nullptr;
    }

    if (parent == nullptr || parent->is_document()) {
        return m_document->add_child(child);
    }

    if (auto* parent_element = const_cast<Element*>(parent->as_element())) {
        return parent_element->add_child(child);
    }
    return nullptr;
}

Node* TreeBuilder::insert_node_before(Node* child, Node* parent, const Node* before) const {
    if (child == nullptr) {
        return nullptr;
    }

    if (parent == nullptr || parent->is_document()) {
        return m_document->insert_child_before(child, before);
    }

    if (auto* parent_element = const_cast<Element*>(parent->as_element())) {
        return parent_element->insert_child_before(child, before);
    }
    return nullptr;
}

std::string_view TreeBuilder::limit_text_for_node(const std::string_view text, const size_t existing_length) {
    if (text.empty()) {
        return {};
    }

    const size_t limit = m_options.max_text_length;
    if (existing_length >= limit) {
        parse_error(ErrorCode::TextTooLong, "Text node length limit exceeded", m_last_position);
        return {};
    }

    const size_t remaining = limit - existing_length;
    if (text.size() > remaining) {
        parse_error(ErrorCode::TextTooLong, "Text node length limit exceeded", m_last_position);
        return text.substr(0, remaining);
    }
    return text;
}

void TreeBuilder::insert_text(std::string_view text) {
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
                auto* text_node = static_cast<TextNode*>(last);
                text            = limit_text_for_node(text, text_node->length());
                if (!text.empty()) {
                    text_node->append_text(text);
                }
                return;
            }
        }
    }

    text = limit_text_for_node(text, 0);
    if (text.empty()) {
        return;
    }

    TextNode* text_node = m_document->create_text(text);
    if (m_element_stack.empty()) {
        m_document->add_child(text_node);
    } else {
        current_element()->add_child(text_node);
    }
}

void TreeBuilder::insert_text_before(std::string_view text, Node* parent, const Node* before) {
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
        auto* text_node = static_cast<TextNode*>(previous);
        text            = limit_text_for_node(text, text_node->length());
        if (!text.empty()) {
            text_node->append_text(text);
        }
        return;
    }

    text = limit_text_for_node(text, 0);
    if (text.empty()) {
        return;
    }

    TextNode* text_node = m_document->create_text(text);
    insert_node_before(text_node, parent, before);
}

void TreeBuilder::insert_comment(std::string_view comment) const {
    CommentNode* comment_node = m_document->create_comment(comment);
    if (m_element_stack.empty()) {
        m_document->add_child(comment_node);
    } else {
        current_element()->add_child(comment_node);
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
    return element != nullptr && std::ranges::find(m_element_stack, element) != m_element_stack.end();
}

void TreeBuilder::close_elements_until(const Tag target, const bool report_auto_close_errors) {
    while (m_element_stack.size() > m_stack_floor) {
        const auto element = m_element_stack.back();
        m_element_stack.pop_back();
        if (tag::is_marker_scope(element->tag())) {
            clear_active_formatting_to_last_marker();
        }
        if (element->tag() == target) {
            break;
        }
        if (report_auto_close_errors && !tag::is_optional_end(element->tag())) {
            parse_error(ErrorCode::MismatchedTag, "Auto-closing unclosed tag: " + std::string(element->tag_name()));
        }
    }
}

void TreeBuilder::close_elements_until_by_name(const std::string_view tag_name, const bool report_auto_close_errors) {
    while (m_element_stack.size() > m_stack_floor) {
        const auto element = m_element_stack.back();
        m_element_stack.pop_back();
        if (tag::is_marker_scope(element->tag())) {
            clear_active_formatting_to_last_marker();
        }
        if (equals_ignore_case(element->tag_name(), tag_name)) {
            break;
        }
        if (report_auto_close_errors && !tag::is_optional_end(element->tag())) {
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

void TreeBuilder::check_implicit_close(const Tag tg) {
    while (m_element_stack.size() > m_stack_floor) {
        const auto current = current_element();
        const Tag  cur     = current->tag();

        // <p> 会被这些块级起始标签隐式关闭（HTML5「have a p element in button scope」组，
        // 含 h1–h6/pre/listing/form/plaintext/xmp/table/hr 等）。
        const bool closes_paragraph = cur == tag::kP && tag::is_p_closer(tg);
        const bool closes_list_item = (cur == tag::kLi && tg == tag::kLi) || ((cur == tag::kDd || cur == tag::kDt) && (tg == tag::kDd || tg == tag::kDt));
        const bool closes_button    = cur == tag::kButton && tg == tag::kButton;
        // 一个 h1–h6 起始标签会关闭当前打开的 h1–h6（标题不可嵌套）。
        const bool closes_heading       = tag::is_heading(cur) && tag::is_heading(tg);
        const bool closes_table_cell    = tag::is_table_cell(cur) && (tag::is_table_cell(tg) || tg == tag::kTr || tag::is_table_section(tg));
        const bool closes_table_row     = cur == tag::kTr && (tg == tag::kTr || tag::is_table_section(tg) || tg == tag::kTable);
        const bool closes_table_section = tag::is_table_section(cur) && (tag::is_table_section(tg) || tg == tag::kTable);
        const bool should_pop_current   = closes_paragraph || closes_list_item || closes_button || closes_heading || closes_table_cell || closes_table_row || closes_table_section;

        if (should_pop_current) {
            m_element_stack.pop_back();
            if (tag::is_marker_scope(cur)) {
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

    Element* html_element = m_document->create_element("html");
    m_html_element        = const_cast<Element*>(insert_node(html_element, m_document.get())->as_element());
    push_if_absent(m_html_element);
}

void TreeBuilder::ensure_head_element() {
    ensure_html_element();

    if (!m_head_element) {
        Element* head_element = m_document->create_element("head");
        m_head_element        = const_cast<Element*>(insert_node(head_element, m_html_element)->as_element());
    }

    m_head_closed = false;
    if (current_element() == nullptr || current_element() == m_html_element) {
        push_if_absent(m_head_element);
    }
}

void TreeBuilder::ensure_body_element() {
    ensure_html_element();

    if (!m_head_element) {
        Element* head_element = m_document->create_element("head");
        m_head_element        = const_cast<Element*>(insert_node(head_element, m_html_element)->as_element());
        m_head_closed         = true;
    } else if (!m_head_closed) {
        close_head_element_if_open();
    }

    if (!m_body_element) {
        Element* body_element = m_document->create_element("body");
        m_body_element        = const_cast<Element*>(insert_node(body_element, m_html_element)->as_element());
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
        close_elements_until(tag::kHead, false);
    }
    m_head_closed = true;
}

void TreeBuilder::prepare_table_context_for_start_tag(const Tag tg) {
    if (tag::is_table_structure(tg)) {
        close_foster_parented_elements_before_table_token();
    }

    if (tg == tag::kCaption) {
        close_open_table_content_before_container(tag::kCaption);
        return;
    }

    if (tg == tag::kColgroup) {
        close_open_table_content_before_container(tag::kColgroup);
        return;
    }

    if (tg == tag::kCol) {
        close_open_table_content_before_container(tag::kColgroup, false);
        ensure_colgroup();
        return;
    }

    if (tag::is_table_section(tg) || tg == tag::kTr || tag::is_table_cell(tg)) {
        if (find_open_element(tag::kCaption, false) != nullptr) {
            close_elements_until(tag::kCaption, false);
        }
        if (find_open_element(tag::kColgroup, false) != nullptr) {
            close_elements_until(tag::kColgroup, false);
        }
    }

    if (tg == tag::kTr) {
        ensure_table_section();
        return;
    }

    if (tag::is_table_cell(tg)) {
        ensure_table_row();
    }
}

void TreeBuilder::prepare_select_context_for_start_tag(const Tag tg) {
    if (tg == tag::kOption) {
        if (find_open_in_select_scope(tag::kOption) != nullptr) {
            close_elements_until(tag::kOption, false);
        }
        return;
    }

    if (tg == tag::kOptgroup) {
        if (find_open_in_select_scope(tag::kOption) != nullptr) {
            close_elements_until(tag::kOption, false);
        }
        if (find_open_in_select_scope(tag::kOptgroup) != nullptr) {
            close_elements_until(tag::kOptgroup, false);
        }
        return;
    }

    if (tg == tag::kSelect) {
        if (find_open_in_select_scope(tag::kOption) != nullptr) {
            close_elements_until(tag::kOption, false);
        }
        if (find_open_in_select_scope(tag::kOptgroup) != nullptr) {
            close_elements_until(tag::kOptgroup, false);
        }
        if (find_open_in_select_scope(tag::kSelect) != nullptr) {
            close_elements_until(tag::kSelect, false);
        }
    }
}

bool TreeBuilder::handle_table_end_tag(const Tag tg) {
    if (tg == tag::kTable || tg == tag::kTr || tag::is_table_section(tg) || tag::is_table_cell(tg) || tg == tag::kCaption || tg == tag::kColgroup) {
        close_foster_parented_elements_before_table_token();
    }

    if (tg == tag::kCaption || tg == tag::kColgroup) {
        if (find_open_element(tg, false) == nullptr) {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: " + std::string(tag::static_name(tg)));
            return true;
        }
        close_elements_until(tg, false);
        return true;
    }

    if (tag::is_table_cell(tg)) {
        if (find_open_element(tg, false) == nullptr) {
            return false;
        }
        close_elements_until(tg, false);
        return true;
    }

    if (tg == tag::kTr) {
        if (find_open_table_cell() != nullptr) {
            close_elements_until(find_open_table_cell()->tag(), false);
        }
        if (find_open_table_row() == nullptr) {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: tr");
            return true;
        }
        close_elements_until(tag::kTr, false);
        return true;
    }

    if (tag::is_table_section(tg)) {
        if (find_open_table_cell() != nullptr) {
            close_elements_until(find_open_table_cell()->tag(), false);
        }
        if (find_open_table_row() != nullptr) {
            close_elements_until(tag::kTr, false);
        }
        if (find_open_element(tg, false) == nullptr) {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: " + std::string(tag::static_name(tg)));
            return true;
        }
        close_elements_until(tg, false);
        return true;
    }

    if (tg == tag::kTable) {
        if (find_open_element(tag::kCaption, false) != nullptr) {
            close_elements_until(tag::kCaption, false);
        }
        if (find_open_element(tag::kColgroup, false) != nullptr) {
            close_elements_until(tag::kColgroup, false);
        }
        if (find_open_table_cell() != nullptr) {
            close_elements_until(find_open_table_cell()->tag(), false);
        }
        if (find_open_table_row() != nullptr) {
            close_elements_until(tag::kTr, false);
        }
        if (find_open_table_section() != nullptr) {
            close_elements_until(find_open_table_section()->tag(), false);
        }
        if (find_open_element(tag::kTable, false) == nullptr) {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: table");
            return true;
        }
        close_elements_until(tag::kTable, false);
        return true;
    }

    return false;
}

bool TreeBuilder::handle_select_end_tag(const Tag tg) {
    if (tg == tag::kOption) {
        if (find_open_in_select_scope(tag::kOption) == nullptr) {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: option");
            return true;
        }
        close_elements_until(tag::kOption, false);
        return true;
    }

    if (tg == tag::kOptgroup) {
        if (find_open_in_select_scope(tag::kOption) != nullptr) {
            close_elements_until(tag::kOption, false);
        }
        if (find_open_in_select_scope(tag::kOptgroup) == nullptr) {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: optgroup");
            return true;
        }
        close_elements_until(tag::kOptgroup, false);
        return true;
    }

    if (tg == tag::kSelect) {
        if (find_open_in_select_scope(tag::kOption) != nullptr) {
            close_elements_until(tag::kOption, false);
        }
        if (find_open_in_select_scope(tag::kOptgroup) != nullptr) {
            close_elements_until(tag::kOptgroup, false);
        }
        if (find_open_in_select_scope(tag::kSelect) == nullptr) {
            parse_error(ErrorCode::MismatchedTag, "No matching opening tag for: select");
            return true;
        }
        close_elements_until(tag::kSelect, false);
        return true;
    }

    return false;
}

void TreeBuilder::close_open_table_content_before_container(const Tag container, const bool close_matching_tag) {
    if (find_open_table_cell() != nullptr) {
        close_elements_until(find_open_table_cell()->tag(), false);
    }
    if (find_open_element(tag::kTr, false) != nullptr) {
        close_elements_until(tag::kTr, false);
    }
    if (find_open_table_section() != nullptr) {
        close_elements_until(find_open_table_section()->tag(), false);
    }

    for (const Tag c : {tag::kCaption, tag::kColgroup}) {
        if (c != container && find_open_element(c, false) != nullptr) {
            close_elements_until(c, false);
        }
    }

    if (close_matching_tag && find_open_element(container, false) != nullptr) {
        close_elements_until(container, false);
    }
}

void TreeBuilder::ensure_table_section(const std::string_view tag_name) {
    if (find_open_element(tag::kTable) == nullptr || find_open_table_section() != nullptr) {
        return;
    }

    Element* section     = m_document->create_element(tag_name);
    auto*    section_ptr = const_cast<Element*>(insert_node(section, find_open_element(tag::kTable))->as_element());
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

    Element* row     = m_document->create_element("tr");
    auto*    row_ptr = const_cast<Element*>(insert_node(row, section)->as_element());
    push_if_absent(row_ptr);
}

void TreeBuilder::ensure_colgroup() {
    if (find_open_element(tag::kTable) == nullptr || find_open_element(tag::kColgroup) != nullptr) {
        return;
    }

    Element* colgroup     = m_document->create_element("colgroup");
    auto*    colgroup_ptr = const_cast<Element*>(insert_node(colgroup, find_open_element(tag::kTable))->as_element());
    push_if_absent(colgroup_ptr);
}

void TreeBuilder::close_colgroup_for_non_col_token() {
    if (current_element() == nullptr) {
        return;
    }
    if (current_element()->tag() != tag::kColgroup) {
        return;
    }
    close_elements_until(tag::kColgroup, false);
}

NamespaceKind TreeBuilder::current_insertion_namespace() const noexcept {
    const auto* current = current_element();
    if (current == nullptr) {
        return NamespaceKind::Html;
    }
    const NamespaceKind ns = current->namespace_kind();
    const Tag           t  = current->tag();
    if (ns == NamespaceKind::Svg) {
        // SVG HTML 集成点：foreignObject / desc / title 的内容按 HTML 解析。
        if (t == tag::kForeignObject || t == tag::kDesc || t == tag::kTitle) {
            return NamespaceKind::Html;
        }
    } else if (ns == NamespaceKind::MathML) {
        // MathML 文本集成点：mi/mo/mn/ms/mtext 的内容按 HTML 解析。
        if (t == tag::kMi || t == tag::kMo || t == tag::kMn || t == tag::kMs || t == tag::kMtext) {
            return NamespaceKind::Html;
        }
    }
    return ns;
}

NamespaceKind TreeBuilder::namespace_for_start_tag(const Tag tg) const noexcept {
    const auto inherited_namespace = current_insertion_namespace();
    if (tg == tag::kSvg) {
        return NamespaceKind::Svg;
    }
    if (tg == tag::kMath) {
        return NamespaceKind::MathML;
    }
    if (inherited_namespace != NamespaceKind::Html) {
        return inherited_namespace;
    }
    return NamespaceKind::Html;
}

bool TreeBuilder::is_all_whitespace(const std::string_view text) noexcept {
    return std::ranges::all_of(text, is_whitespace);
}

bool TreeBuilder::should_foster_parent_text() const noexcept {
    const auto* current = current_element();
    if (current == nullptr) {
        return false;
    }
    const Tag t = current->tag();
    return t == tag::kTable || tag::is_table_section(t) || t == tag::kTr;
}

bool TreeBuilder::should_foster_parent_element(const Tag tg) const noexcept {
    if (tag::is_table_structure(tg)) {
        return false;
    }
    // <template> 在表格内原位插入（不 foster），并切换到 “in template” 隔离上下文。
    if (tg == tag::kTemplate) {
        return false;
    }

    const auto* current = current_element();
    if (current == nullptr) {
        return false;
    }
    const Tag t = current->tag();
    return t == tag::kTable || tag::is_table_section(t) || t == tag::kTr;
}

std::pair<Node*, const Node*> TreeBuilder::foster_parent_insertion_point() const noexcept {
    Element* table = find_open_element(tag::kTable);
    if (table == nullptr) {
        return {current_element() != nullptr ? static_cast<Node*>(current_element()) : static_cast<Node*>(m_document.get()), nullptr};
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
    Element* table = find_open_element(tag::kTable);
    if (table == nullptr) {
        return;
    }

    while (m_element_stack.size() > m_stack_floor) {
        Element* current = current_element();
        if (current == nullptr || current == table) {
            return;
        }
        if (tag::is_table_structure(current->tag())) {
            return;
        }
        m_element_stack.pop_back();
    }
}

// ==================== 活动格式化元素列表 + 领养机构算法 ====================

bool TreeBuilder::is_special_element(const Element& element) noexcept {
    const Tag t = element.tag();
    if (element.namespace_kind() != NamespaceKind::Html) {
        // 外来内容里的集成点元素亦视作 special（scope 边界）。
        return t == tag::kForeignObject || t == tag::kDesc || t == tag::kTitle || t == tag::kMi || t == tag::kMo || t == tag::kMn || t == tag::kMs || t == tag::kMtext || t == tag::kAnnotationXml;
    }
    return tag::is_special(t);
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
        const bool found = std::ranges::any_of(ba, [&attr](const auto& other) { return other.name() == attr.name() && other.value() == attr.value(); });
        if (!found) {
            return false;
        }
    }
    return true;
}

bool TreeBuilder::is_in_active_formatting(const Element* element) const noexcept {
    return element != nullptr && std::ranges::find(m_active_formatting, element) != m_active_formatting.end();
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

Element* TreeBuilder::insert_html_element_at_current(Element* element) {
    Node* parent = m_element_stack.empty() ? static_cast<Node*>(m_document.get()) : static_cast<Node*>(current_element());
    return const_cast<Element*>(insert_node(element, parent)->as_element());
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
        const Tag t = element->tag();
        if (element->namespace_kind() == NamespaceKind::Html) {
            if (t == tag::kApplet || t == tag::kCaption || t == tag::kHtml || t == tag::kTable || tag::is_table_cell(t) || t == tag::kMarquee || t == tag::kObject || t == tag::kTemplate) {
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
    old_parent->remove_child(node);
    if (before != nullptr) {
        new_parent->insert_child_before(node, before);
    } else {
        new_parent->add_child(node);
    }
}
}  // namespace

bool TreeBuilder::run_adoption_agency(const Tag subject) {
    // 1. 当前节点即 subject 且不在活动格式化列表：直接弹出。
    if (Element* current = current_element(); current != nullptr && current->namespace_kind() == NamespaceKind::Html && current->tag() == subject && !is_in_active_formatting(current)) {
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
            if (entry->tag() == subject) {
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
            auto* clone = const_cast<Element*>(common_ancestor->add_child(clone_element_shallow(*node))->as_element());
            if (const auto afe_it = std::ranges::find(m_active_formatting, node); afe_it != m_active_formatting.end()) {
                *afe_it = clone;
            }
            m_element_stack[node_index] = clone;
            node                        = clone;

            if (last_node == furthest_block) {
                bookmark = static_cast<size_t>(std::ranges::find(m_active_formatting, node) - m_active_formatting.begin()) + 1;
            }

            // 把 last_node 挂到 node 之下。
            move_node_into(last_node, node, nullptr);
            last_node = node;
        }

        // 9. 把 last_node 放到 common ancestor 的“合适位置”（表格上下文需 foster）。
        if (const Tag ca = common_ancestor->tag(); should_foster_parent_element(ca) || ca == tag::kTable || tag::is_table_section(ca) || ca == tag::kTr) {
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
        auto* fe_clone = const_cast<Element*>(furthest_block->add_child(clone_element_shallow(*formatting_element))->as_element());
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
        m_active_formatting.insert(m_active_formatting.begin() + static_cast<std::ptrdiff_t>(bookmark), fe_clone);

        if (const auto fe_stk = std::ranges::find(m_element_stack, formatting_element); fe_stk != m_element_stack.end()) {
            m_element_stack.erase(fe_stk);
        }
        if (const auto fb_stk = std::ranges::find(m_element_stack, furthest_block); fb_stk != m_element_stack.end()) {
            m_element_stack.insert(fb_stk + 1, fe_clone);
        }
    }
    return true;
}

Element* TreeBuilder::find_open_element(const Tag target, const bool include_fragment_base) const noexcept {
    for (size_t index = m_element_stack.size(); index > 0; --index) {
        if (!include_fragment_base && index <= m_stack_floor) {
            break;
        }
        Element* element = m_element_stack[index - 1];
        if (element->tag() == target) {
            return element;
        }
    }
    return nullptr;
}

Element* TreeBuilder::find_open_in_select_scope(const Tag target) const noexcept {
    for (size_t index = m_element_stack.size(); index > 0; --index) {
        if (index <= m_stack_floor) {
            break;
        }
        Element* element = m_element_stack[index - 1];
        if (element->tag() == target) {
            return element;
        }
        if (element->tag() == tag::kSelect) {
            return target == tag::kSelect ? element : nullptr;
        }
    }
    return nullptr;
}

Element* TreeBuilder::find_open_table_section() const noexcept {
    for (size_t index = m_element_stack.size(); index > 0; --index) {
        Element* element = m_element_stack[index - 1];
        if (tag::is_table_section(element->tag())) {
            return element;
        }
        if (element->tag() == tag::kTable) {
            break;
        }
    }
    return nullptr;
}

Element* TreeBuilder::find_open_table_row() const noexcept {
    for (size_t index = m_element_stack.size(); index > 0; --index) {
        Element* element = m_element_stack[index - 1];
        if (element->tag() == tag::kTr) {
            return element;
        }
        if (element->tag() == tag::kTable) {
            break;
        }
    }
    return nullptr;
}

Element* TreeBuilder::find_open_table_cell() const noexcept {
    for (size_t index = m_element_stack.size(); index > 0; --index) {
        Element* element = m_element_stack[index - 1];
        if (tag::is_table_cell(element->tag())) {
            return element;
        }
        if (element->tag() == tag::kTable) {
            break;
        }
    }
    return nullptr;
}

}  // namespace hps
