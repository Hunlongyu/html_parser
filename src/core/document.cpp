#include "hps/core/document.hpp"

#include "hps/core/comment_node.hpp"
#include "hps/core/doctype_node.hpp"
#include "hps/core/element.hpp"
#include "hps/core/tag_table.hpp"
#include "hps/core/text_node.hpp"
#include "hps/query/element_query.hpp"
#include "hps/query/query.hpp"
#include "hps/utils/string_utils.hpp"
#include "hps/utils/url.hpp"

#include "serializer.hpp"

#include <algorithm>
#include <sstream>

namespace hps {
namespace {
std::string normalize_tag_key(const std::string_view tag_name) {
    std::string normalized(tag_name);
    std::ranges::transform(normalized, normalized.begin(), [](const unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return normalized;
}

}  // namespace

Document::Document(std::string html_content, std::string base_url)
    : Node(NodeType::Document),
      m_html_source(std::move(html_content)),
      m_base_url(std::move(base_url)) {
    // 文档自身的 owner 即自身；其 arena/池创建的节点在工厂里写入各自的 owner。
    m_owner_document = this;
}

std::string_view Document::intern(const std::string_view s) {
    if (s.empty()) {
        return {};
    }
    // 已是源码（m_html_source）子串 → 零拷贝；否则拷入字符串池。两者都与本文档同生命周期。
    const char* base = m_html_source.data();
    if (!m_html_source.empty() && s.data() >= base && s.data() + s.size() <= base + m_html_source.size()) {
        return s;
    }
    return m_strings.add(s);
}

NodeType Document::type() const noexcept {
    return NodeType::Document;
}

void Document::invalidate_query_indexes() noexcept {
    m_query_index_cache.id_lookup.clear();
    m_query_index_cache.class_lookup.clear();
    m_query_index_cache.tag_lookup.clear();
    m_query_index_cache.valid = false;
    m_cached_title.reset();
    m_cached_charset.reset();
    m_cached_base_url.reset();
}

void Document::index_element_subtree(const Element& element) const {
    if (!element.id().empty()) {
        m_query_index_cache.id_lookup.emplace(element.id(), &element);
    }
    m_query_index_cache.tag_lookup[normalize_tag_key(element.tag_name())].push_back(&element);

    for (const std::string_view class_name : element.class_names()) {
        m_query_index_cache.class_lookup[std::string(class_name)].push_back(&element);
    }

    for (auto child = element.first_child(); child; child = child->next_sibling()) {
        if (const auto* child_element = child->as_element()) {
            index_element_subtree(*child_element);
        }
    }
}

void Document::ensure_query_indexes() const {
    if (m_query_index_cache.valid) {
        return;
    }

    m_query_index_cache.id_lookup.clear();
    m_query_index_cache.class_lookup.clear();
    m_query_index_cache.tag_lookup.clear();

    for (auto child = first_child(); child; child = child->next_sibling()) {
        if (const auto* child_element = child->as_element()) {
            index_element_subtree(*child_element);
        }
    }

    m_query_index_cache.valid = true;
}

std::string Document::text_content() const {
    std::stringstream ss;
    for (auto child = first_child(); child; child = child->next_sibling()) {
        ss << child->text_content();
    }
    return ss.str();
}

std::string Document::title() const {
    if (m_cached_title.has_value()) {
        return m_cached_title.value();
    }
    if (const auto elements = get_elements_by_tag_name("title"); !elements.empty()) {
        m_cached_title = elements.at(0)->text_content();
    } else {
        m_cached_title = std::string{};
    }
    return m_cached_title.value();
}

std::string Document::charset() const {
    if (m_cached_charset.has_value()) {
        return m_cached_charset.value();
    }
    const auto meta_elements = get_elements_by_tag_name("meta");
    for (const auto& meta : meta_elements) {
        if (meta->has_attribute("charset")) {
            m_cached_charset = std::string(trim_whitespace(meta->get_attribute("charset")));
            return m_cached_charset.value();
        }
        const auto http_equiv = trim_whitespace(meta->get_attribute("http-equiv"));
        if (!http_equiv.empty() && equals_ignore_case(http_equiv, "Content-Type")) {
            const std::string_view content = trim_whitespace(meta->get_attribute("content"));
            auto                   pos     = content.find("charset=");
            if (pos == std::string_view::npos) {
                for (size_t i = 0; i + 8 <= content.size(); ++i) {
                    if (starts_with_ignore_case(content.substr(i), "charset=")) {
                        pos = i;
                        break;
                    }
                }
            }
            if (pos != std::string_view::npos) {
                const size_t start   = pos + 8;
                const size_t end_pos = content.find_first_of("; \t\n\r", start);
                const auto   value   = trim_whitespace(content.substr(start, end_pos == std::string_view::npos ? std::string_view::npos : end_pos - start));
                m_cached_charset     = std::string(value);
                return m_cached_charset.value();
            }
        }
    }
    m_cached_charset = std::string{};
    return m_cached_charset.value();
}

std::string_view Document::source_html() const noexcept {
    return m_html_source;
}

std::string Document::outer_html() const {
    std::string out;
    detail::serialize_children(*this, out);
    return out;
}

std::string Document::get_meta_content(const std::string_view name) const {
    const auto meta_elements = get_elements_by_tag_name("meta");
    for (const auto& meta : meta_elements) {
        const auto meta_name = trim_whitespace(meta->get_attribute("name"));
        if (!meta_name.empty() && equals_ignore_case(meta_name, name)) {
            return std::string(meta->get_attribute("content"));
        }
    }
    return {};
}

std::string Document::get_meta_property(const std::string_view property) const {
    const auto meta_elements = get_elements_by_tag_name("meta");
    for (const auto& meta : meta_elements) {
        const auto meta_property = trim_whitespace(meta->get_attribute("property"));
        if (!meta_property.empty() && equals_ignore_case(meta_property, property)) {
            return std::string(meta->get_attribute("content"));
        }
    }
    return {};
}

const std::string& Document::base_url() const {
    if (!m_cached_base_url.has_value()) {
        std::string effective = m_base_url;
        // <base href> 覆盖/补全文档基准（自身相对 Options::base_url 解析）。
        if (const Element* base = query_selector("base"); base != nullptr) {
            if (const auto href = base->attr("href"); href.has_value() && !href->empty()) {
                effective = hps::resolve_url(m_base_url, *href);
            }
        }
        m_cached_base_url = std::move(effective);
    }
    return *m_cached_base_url;
}

std::string Document::resolve_url(const std::string_view reference) const {
    return hps::resolve_url(base_url(), reference);
}

std::vector<std::string> Document::get_all_links() const {
    const std::string& base  = base_url();
    auto               links = Query::css(*this, "a[href]").extract_attributes("href");
    for (auto& link : links) {
        link = hps::resolve_url(base, link);
    }
    return links;
}

std::vector<std::string> Document::get_all_images() const {
    const std::string& base   = base_url();
    auto               images = Query::css(*this, "img[src]").extract_attributes("src");
    for (auto& image : images) {
        image = hps::resolve_url(base, image);
    }
    return images;
}

const Element* Document::root() const {
    if (const auto html_element = html()) {
        return html_element;
    }
    for (auto child = first_child(); child; child = child->next_sibling()) {
        if (child->is_element()) {
            return child->as_element();
        }
    }
    return nullptr;
}

const Element* Document::html() const {
    for (auto child = first_child(); child; child = child->next_sibling()) {
        if (child->is_element()) {
            const auto element = child->as_element();
            if (equals_ignore_case(element->tag_name(), "html")) {
                return element;
            }
        }
    }
    return nullptr;
}

const Element* Document::query_selector(const std::string_view selector) const {
    return Query::css_first(*this, selector);
}

std::vector<const Element*> Document::query_selector_all(const std::string_view selector) const {
    return Query::css(*this, selector).elements();
}

const Element* Document::get_element_by_id(const std::string_view id) const {
    if (id.empty()) {
        return nullptr;
    }

    ensure_query_indexes();

    if (const auto it = m_query_index_cache.id_lookup.find(std::string(id)); it != m_query_index_cache.id_lookup.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<const Element*> Document::get_elements_by_tag_name(const std::string_view tag_name) const {
    ensure_query_indexes();

    if (const auto it = m_query_index_cache.tag_lookup.find(normalize_tag_key(tag_name)); it != m_query_index_cache.tag_lookup.end()) {
        return it->second;
    }
    return {};
}

std::vector<const Element*> Document::get_elements_by_class_name(const std::string_view class_name) const {
    ensure_query_indexes();

    if (const auto it = m_query_index_cache.class_lookup.find(std::string(class_name)); it != m_query_index_cache.class_lookup.end()) {
        return it->second;
    }
    return {};
}

ElementQuery Document::css(const std::string_view selector) const {
    return Query::css(*this, selector);
}

Element* Document::create_element(const std::string_view name, const NamespaceKind ns) {
    // 逐字小写的已知标签 → 用静态名（零池零拷贝）；其余（自定义/保留大小写/外来）驻留。
    const Tag              t      = tag::from_name_ci(name);
    const std::string_view stable = (t != Tag::Unknown && name == tag::static_name(t)) ? tag::static_name(t) : intern(name);
    Element* element          = m_arena.make<Element>(stable, ns);
    element->m_owner_document = this;
    return element;
}

TextNode* Document::create_text(const std::string_view text) {
    // 驻留为稳定视图：源码子串零拷贝，否则入池（bump，比 std::string malloc 更省）。
    // 文本节点默认零拷贝持有此视图；若后续相邻文本合并 append，再物化为自有缓冲。
    TextNode* node         = m_arena.make<TextNode>(intern(text));
    node->m_owner_document = this;
    return node;
}

CommentNode* Document::create_comment(const std::string_view text) {
    CommentNode* node      = m_arena.make<CommentNode>(intern(text));
    node->m_owner_document = this;
    return node;
}

DoctypeNode* Document::create_doctype(
    const std::string_view name, const std::string_view public_id, const std::string_view system_id, const bool has_identifiers) {
    DoctypeNode* node      = m_arena.make<DoctypeNode>(name, public_id, system_id, has_identifiers);
    node->m_owner_document = this;
    return node;
}

Node* Document::add_child(Node* child) {
    if (child == nullptr) {
        return nullptr;
    }
    Node* inserted = append_child(child);
    invalidate_query_indexes();
    return inserted;
}

Node* Document::insert_child_before(Node* child, const Node* before) {
    if (child == nullptr) {
        return nullptr;
    }
    Node* inserted = Node::insert_child_before(child, before);
    invalidate_query_indexes();
    return inserted;
}

std::vector<Node*> Document::take_children() {
    auto children = Node::take_children();
    invalidate_query_indexes();
    return children;
}
}  // namespace hps
