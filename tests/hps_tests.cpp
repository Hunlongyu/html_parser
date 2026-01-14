#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/core/text_node.hpp"
#include "hps/query/element_query.hpp"

#include <cassert>
#include <memory>
#include <string_view>

using namespace hps;

static void test_document_root_prefers_html() {
    Document doc("");

    auto head_node = std::make_unique<Element>("head");
    auto html_node = std::make_unique<Element>("html");
    const auto* html_ptr = html_node.get();

    doc.add_child(std::move(head_node));
    doc.add_child(std::move(html_node));

    assert(doc.root() == html_ptr);
}

static void test_document_charset_parsing_case_and_whitespace() {
    Document doc("");

    auto html_node = std::make_unique<Element>("html");
    auto head_node = std::make_unique<Element>("head");

    auto meta_equiv = std::make_unique<Element>("meta");
    meta_equiv->add_attribute("http-equiv", "content-type");
    meta_equiv->add_attribute("content", "text/html; Charset=UTF-8");

    auto meta_charset = std::make_unique<Element>("meta");
    meta_charset->add_attribute("charset", "  utf-8  ");

    head_node->add_child(std::move(meta_charset));
    head_node->add_child(std::move(meta_equiv));
    html_node->add_child(std::move(head_node));
    doc.add_child(std::move(html_node));

    assert(doc.charset() == "utf-8");
}

static void test_elementquery_next_prev_sibling_skip_non_elements() {
    Document doc("");
    auto html_node = std::make_unique<Element>("html");

    auto first_node = std::make_unique<Element>("a");
    const auto* first_ptr = first_node.get();
    html_node->add_child(std::move(first_node));

    html_node->add_child(std::make_unique<TextNode>("x"));

    auto second_node = std::make_unique<Element>("b");
    const auto* second_ptr = second_node.get();
    html_node->add_child(std::move(second_node));

    doc.add_child(std::move(html_node));

    ElementQuery q(first_ptr);
    assert(q.next_sibling().first_element() == second_ptr);

    ElementQuery q2(second_ptr);
    assert(q2.prev_sibling().first_element() == first_ptr);
}

static void test_elementquery_siblings_keeps_document_order() {
    Document doc("");
    auto html_node = std::make_unique<Element>("html");

    auto a_node = std::make_unique<Element>("a");
    const auto* a_ptr = a_node.get();
    html_node->add_child(std::move(a_node));
    html_node->add_child(std::make_unique<TextNode>("x"));

    auto b_node = std::make_unique<Element>("b");
    const auto* b_ptr = b_node.get();
    html_node->add_child(std::move(b_node));
    html_node->add_child(std::make_unique<TextNode>("y"));

    auto c_node = std::make_unique<Element>("c");
    const auto* c_ptr = c_node.get();
    html_node->add_child(std::move(c_node));

    doc.add_child(std::move(html_node));

    ElementQuery q(b_ptr);
    const auto siblings = q.siblings().elements();
    assert(siblings.size() == 2);
    assert(siblings[0] == a_ptr);
    assert(siblings[1] == c_ptr);
}

int main() {
    test_document_root_prefers_html();
    test_document_charset_parsing_case_and_whitespace();
    test_elementquery_next_prev_sibling_skip_non_elements();
    test_elementquery_siblings_keeps_document_order();
    return 0;
}

