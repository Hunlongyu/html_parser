#include "hps/core/document.hpp"

#include "hps/core/element.hpp"
#include "hps/query/element_query.hpp"
#include "hps/utils/string_utils.hpp"

#include <algorithm>
#include <sstream>

namespace hps {
Document::Document(std::string html_content) : Node(NodeType::Document), m_html_source(std::move(html_content)) {}

NodeType Document::type() {
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
    if (const auto elements = get_elements_by_tag_name("title"); !elements.empty()) {
        return elements.at(0)->text_content();
    }
    return {};
}

std::string Document::charset() const {
    const auto meta_elements = get_elements_by_tag_name("meta");
    for (const auto& meta : meta_elements) {
        if (meta->has_attribute("charset")) {
            return meta->get_attribute("charset");
        }
        if (meta->get_attribute("http-equiv") == "Content-Type") {
            const std::string content     = meta->get_attribute("content");
            const size_t      charset_pos = content.find("charset=");
            if (charset_pos != std::string::npos) {
                const size_t start = charset_pos + 8;
                const size_t end   = content.find_first_of("; \t\n\r", start);
                return content.substr(start, end == std::string::npos ? std::string::npos : end - start);
            }
        }
    }
    return {};
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
    std::vector<std::string> links;
    const auto               a_elements = get_elements_by_tag_name("a");
    for (const auto& a : a_elements) {
        if (a->has_attribute("href")) {
            const std::string href = a->get_attribute("href");
            if (!href.empty()) {
                links.push_back(href);
            }
        }
    }
    return links;
}

std::vector<std::string> Document::get_all_images() const {
    std::vector<std::string> images;
    const auto               img_elements = get_elements_by_tag_name("img");
    for (const auto& img : img_elements) {
        if (img->has_attribute("src")) {
            const std::string src = img->get_attribute("src");
            if (!src.empty()) {
                images.push_back(src);
            }
        }
    }
    return images;
}

std::shared_ptr<const Element> Document::root() const {
    for (const auto& child : children()) {
        if (child->is_element()) {
            return std::static_pointer_cast<const Element>(child);
        }
    }
    return nullptr;
}

std::shared_ptr<const Element> Document::html() const {
    for (const auto& child : children()) {
        if (child->is_element()) {
            const auto element = std::static_pointer_cast<const Element>(child);
            if (element->tag_name() == "html") {
                return element;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<const Element> Document::querySelector(const std::string_view selector) const {
    // TODO: 实现完整的 CSS 选择器解析
    const auto elements = querySelectorAll(selector);
    return elements.empty() ? nullptr : elements.front();
}

std::vector<std::shared_ptr<const Element>> Document::querySelectorAll(const std::string_view selector) const {
    // TODO: 实现完整的 CSS 选择器解析
    std::vector<std::shared_ptr<const Element>> result;

    if (selector.empty()) {
        return result;
    }

    // 简单的选择器实现
    if (selector[0] == '#') {
        // ID 选择器
        const std::string id(selector.substr(1));
        if (const auto element = get_element_by_id(id)) {
            result.push_back(element);
        }
    } else if (selector[0] == '.') {
        // 类选择器
        const std::string class_name(selector.substr(1));
        result = get_elements_by_class_name(class_name);
    } else {
        // 标签选择器
        result = get_elements_by_tag_name(selector);
    }

    return result;
}

std::shared_ptr<const Element> Document::get_element_by_id(const std::string_view id) const {
    // 递归搜索所有子节点
    std::function<std::shared_ptr<const Element>(const std::shared_ptr<const Node>&)> search_recursive;
    search_recursive = [&](const std::shared_ptr<const Node>& node) -> std::shared_ptr<const Element> {
        if (node->is_element()) {
            const auto element = std::static_pointer_cast<const Element>(node);
            if (element->id() == id) {
                return element;
            }
        }

        for (const auto& child : node->children()) {
            if (const auto found = search_recursive(child)) {
                return found;
            }
        }
        return nullptr;
    };

    for (const auto& child : children()) {
        if (const auto found = search_recursive(child)) {
            return found;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<const Element>> Document::get_elements_by_tag_name(const std::string_view tag_name) const {
    std::vector<std::shared_ptr<const Element>> result;

    // 递归搜索所有子节点
    std::function<void(const std::shared_ptr<const Node>&)> search_recursive;
    search_recursive = [&](const std::shared_ptr<const Node>& node) {
        if (node->is_element()) {
            const auto element = std::static_pointer_cast<const Element>(node);
            if (element->tag_name() == tag_name) {
                result.push_back(element);
            }
        }

        for (const auto& child : node->children()) {
            search_recursive(child);
        }
    };

    for (const auto& child : children()) {
        search_recursive(child);
    }
    return result;
}

std::vector<std::shared_ptr<const Element>> Document::get_elements_by_class_name(const std::string_view class_name) const {
    std::vector<std::shared_ptr<const Element>> result;

    // 递归搜索所有子节点
    std::function<void(const std::shared_ptr<const Node>&)> search_recursive;
    search_recursive = [&](const std::shared_ptr<const Node>& node) {
        if (node->is_element()) {
            const auto element = std::static_pointer_cast<const Element>(node);
            if (element->has_class(class_name)) {
                result.push_back(element);
            }
        }

        for (const auto& child : node->children()) {
            search_recursive(child);
        }
    };

    for (const auto& child : children()) {
        search_recursive(child);
    }
    return result;
}

ElementQuery Document::css(const std::string_view selector) const {
    return ElementQuery(querySelectorAll(selector));
}

ElementQuery Document::xpath(std::string_view expression) const {
    // TODO: 实现 XPath 表达式解析
    // 目前返回空的 ElementQuery
    return ElementQuery();
}

void Document::add_child(const std::shared_ptr<Node>& child) {
    if (!child) {
        return;
    }
    append_child(child);
}
}  // namespace hps