#include "hps/core/document.hpp"
#include "hps/core/comment_node.hpp"
#include "hps/core/element.hpp"
#include "hps/core/text_node.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace hps::tests {

TEST(DocumentTest, SourceHtmlIsStored) {
    const std::string html = "<html></html>";
    const Document    doc(html);
    EXPECT_EQ(doc.source_html(), html);
}

TEST(DocumentTest, RootPrefersHtmlElement) {
    Document doc("");

    Element* head = doc.create_element("head");
    Element* html = doc.create_element("html");
    const auto* html_ptr = html;

    doc.add_child(head);
    doc.add_child(html);

    EXPECT_EQ(doc.html(), html_ptr);
    EXPECT_EQ(doc.root(), html_ptr);
}

TEST(DocumentTest, TextContentConcatenatesAllChildrenRecursively) {
    Document doc("");

    Element* html = doc.create_element("html");
    Element* body = doc.create_element("body");
    body->add_child(doc.create_text("Hello"));
    body->add_child(doc.create_comment("ignored"));
    body->add_child(doc.create_element("span"));
    body->add_child(doc.create_text("World"));
    html->add_child(body);
    doc.add_child(html);

    EXPECT_EQ(doc.text_content(), "HelloWorld");
}

TEST(DocumentTest, CharsetParsesMetaVariants) {
    Document doc("");

    Element* html = doc.create_element("html");
    Element* head = doc.create_element("head");

    Element* meta_equiv = doc.create_element("meta");
    meta_equiv->add_attribute("http-equiv", "content-type");
    meta_equiv->add_attribute("content", "text/html; Charset=UTF-8");

    Element* meta_charset = doc.create_element("meta");
    meta_charset->add_attribute("charset", "  utf-8  ");

    head->add_child(meta_charset);
    head->add_child(meta_equiv);
    html->add_child(head);
    doc.add_child(html);

    EXPECT_EQ(doc.charset(), "utf-8");
    EXPECT_EQ(doc.charset(), "utf-8");
}

TEST(DocumentTest, TitleFindsFirstTitleElement) {
    Document doc("");
    Element* html = doc.create_element("html");
    Element* head = doc.create_element("head");

    Element* title = doc.create_element("title");
    title->add_child(doc.create_text("MyTitle"));
    head->add_child(title);

    html->add_child(head);
    doc.add_child(html);

    EXPECT_EQ(doc.title(), "MyTitle");
    EXPECT_EQ(doc.title(), "MyTitle");
}

TEST(DocumentTest, DirectLookupHelpersDoNotRequireCssEscaping) {
    Document doc("");

    Element* first = doc.create_element("div");
    first->add_attribute("id", "plain");

    Element* second = doc.create_element("section");
    Element* target = doc.create_element("article");
    target->add_attribute("id", "a:b");
    target->add_attribute("class", "entry x:y");
    const auto* target_ptr = target;

    second->add_child(target);
    doc.add_child(first);
    doc.add_child(second);

    EXPECT_EQ(doc.get_element_by_id("a:b"), target_ptr);

    const auto by_class = doc.get_elements_by_class_name("x:y");
    ASSERT_EQ(by_class.size(), 1u);
    EXPECT_EQ(by_class[0], target_ptr);
}

TEST(DocumentTest, QueryIndexesInvalidateAfterAttachedDomMutations) {
    Document doc("");

    Element* root = doc.create_element("div");
    auto* root_ptr = root;
    doc.add_child(root);

    EXPECT_EQ(doc.get_element_by_id("late-id"), nullptr);
    EXPECT_TRUE(doc.get_elements_by_tag_name("span").empty());
    EXPECT_TRUE(doc.get_elements_by_class_name("alpha").empty());

    Element* child = doc.create_element("span");
    child->add_attribute("id", "late-id");
    child->add_attribute("class", "alpha beta");
    auto* child_ptr = child;
    root_ptr->add_child(child);

    EXPECT_EQ(doc.get_element_by_id("late-id"), child_ptr);
    ASSERT_EQ(doc.get_elements_by_tag_name("span").size(), 1u);
    ASSERT_EQ(doc.get_elements_by_class_name("alpha").size(), 1u);

    child_ptr->add_attribute("id", "updated-id");
    child_ptr->add_attribute("class", "gamma");

    EXPECT_EQ(doc.get_element_by_id("late-id"), nullptr);
    EXPECT_EQ(doc.get_element_by_id("updated-id"), child_ptr);
    EXPECT_TRUE(doc.get_elements_by_class_name("alpha").empty());
    ASSERT_EQ(doc.get_elements_by_class_name("gamma").size(), 1u);
}

}  // namespace hps::tests
