#include "hps/core/document.hpp"

#include "hps/core/element.hpp"
#include "hps/query/element_query.hpp"
#include "hps/query/query.hpp"
#include "hps/utils/string_utils.hpp"

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

Document::Document(std::string html_content)
    : Node(NodeType::Document),
      m_html_source(std::move(html_content)) {}

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
}

void Document::index_element_subtree(const Element& element) const {
    if (!element.id().empty()) {
        m_query_index_cache.id_lookup.emplace(element.id(), &element);
    }
    m_query_index_cache.tag_lookup[normalize_tag_key(element.tag_name())].push_back(&element);

    for (const auto& class_name : element.class_names()) {
        m_query_index_cache.class_lookup[class_name].push_back(&element);
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

std::string Document::get_meta_content(const std::string_view name) const {
    const auto meta_elements = get_elements_by_tag_name("meta");
    for (const auto& meta : meta_elements) {
        const auto meta_name = trim_whitespace(meta->get_attribute("name"));
        if (!meta_name.empty() && equals_ignore_case(meta_name, name)) {
            return meta->get_attribute("content");
        }
    }
    return {};
}

std::string Document::get_meta_property(const std::string_view property) const {
    const auto meta_elements = get_elements_by_tag_name("meta");
    for (const auto& meta : meta_elements) {
        const auto meta_property = trim_whitespace(meta->get_attribute("property"));
        if (!meta_property.empty() && equals_ignore_case(meta_property, property)) {
            return meta->get_attribute("content");
        }
    }
    return {};
}

std::vector<std::string> Document::get_all_links() const {
    return Query::css(*this, "a[href]").extract_attributes("href");
}

std::vector<std::string> Document::get_all_images() const {
    return Query::css(*this, "img[src]").extract_attributes("src");
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

const Element* Document::querySelector(const std::string_view selector) const {
    return Query::css_first(*this, selector);
}

std::vector<const Element*> Document::querySelectorAll(const std::string_view selector) const {
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

Node* Document::add_child(std::unique_ptr<Node> child) {
    if (!child) {
        return nullptr;
    }
    Node* inserted = append_child(std::move(child));
    invalidate_query_indexes();
    return inserted;
}

Node* Document::insert_child_before(std::unique_ptr<Node> child, const Node* before) {
    if (!child) {
        return nullptr;
    }
    Node* inserted = Node::insert_child_before(std::move(child), before);
    invalidate_query_indexes();
    return inserted;
}

std::vector<std::unique_ptr<Node>> Document::take_children() {
    auto children = Node::take_children();
    invalidate_query_indexes();
    return children;
}
}  // namespace hps
