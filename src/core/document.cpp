#include "hps/core/document.hpp"

#include "hps/core/element.hpp"
#include "hps/query/element_query.hpp"
#include "hps/query/query.hpp"
#include "hps/utils/string_utils.hpp"

#include <algorithm>
#include <sstream>

namespace hps {
Document::Document(std::string html_content)
    : Node(NodeType::Document),
      m_html_source(std::move(html_content)) {}

NodeType Document::type() const noexcept {
    return NodeType::Document;
}

std::string Document::text_content() const {
    std::stringstream ss;
    for (const auto& child : children()) {
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
            m_cached_charset = meta->get_attribute("charset");
            return m_cached_charset.value();
        }
        if (meta->get_attribute("http-equiv") == "Content-Type") {
            const std::string content     = meta->get_attribute("content");
            const size_t      charset_pos = content.find("charset=");
            if (charset_pos != std::string::npos) {
                const size_t start = charset_pos + 8;
                const size_t end   = content.find_first_of("; \t\n\r", start);
                m_cached_charset   = content.substr(start, end == std::string::npos ? std::string::npos : end - start);
                return m_cached_charset.value();
            }
        }
    }
    m_cached_charset = std::string{};
    return m_cached_charset.value();
}

std::string Document::source_html() const {
    return m_html_source;
}

std::string Document::get_meta_content(const std::string_view name) const {
    const auto meta_elements = get_elements_by_tag_name("meta");
    for (const auto& meta : meta_elements) {
        if (meta->get_attribute("name") == name) {
            return meta->get_attribute("content");
        }
    }
    return {};
}

std::string Document::get_meta_property(const std::string_view property) const {
    const auto meta_elements = get_elements_by_tag_name("meta");
    for (const auto& meta : meta_elements) {
        if (meta->get_attribute("property") == property) {
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

std::shared_ptr<const Element> Document::root() const {
    for (const auto& child : children()) {
        if (child->is_element()) {
            return child->as_element();
        }
    }
    return nullptr;
}

std::shared_ptr<const Element> Document::html() const {
    for (const auto& child : children()) {
        if (child->is_element()) {
            const auto element = child->as_element();
            if (element->tag_name() == "html") {
                return element;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<const Element> Document::querySelector(const std::string_view selector) const {
    return Query::css(*this, selector).first_element();
}

std::vector<std::shared_ptr<const Element>> Document::querySelectorAll(const std::string_view selector) const {
    return Query::css(*this, selector).elements();
}

std::shared_ptr<const Element> Document::get_element_by_id(const std::string_view id) const {
    return Query::css(*this, "#" + std::string(id)).first_element();
}

std::vector<std::shared_ptr<const Element>> Document::get_elements_by_tag_name(const std::string_view tag_name) const {
    return Query::css(*this, std::string(tag_name)).elements();
}

std::vector<std::shared_ptr<const Element>> Document::get_elements_by_class_name(const std::string_view class_name) const {
    return Query::css(*this, "." + std::string(class_name)).elements();
}

ElementQuery Document::css(const std::string_view selector) const {
    return Query::css(*this, selector);
}

ElementQuery Document::xpath(std::string_view expression) const {
    // TODO: 实现 XPath 表达式解析
    return ElementQuery();
}

void Document::add_child(const std::shared_ptr<Node>& child) {
    if (!child) {
        return;
    }
    append_child(child);
}
}  // namespace hps