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

    auto head = std::make_unique<Element>("head");
    auto html = std::make_unique<Element>("html");
    const auto* html_ptr = html.get();

    doc.add_child(std::move(head));
    doc.add_child(std::move(html));

    EXPECT_EQ(doc.html(), html_ptr);
    EXPECT_EQ(doc.root(), html_ptr);
}

TEST(DocumentTest, TextContentConcatenatesAllChildrenRecursively) {
    Document doc("");

    auto html = std::make_unique<Element>("html");
    auto body = std::make_unique<Element>("body");
    body->add_child(std::make_unique<TextNode>("Hello"));
    body->add_child(std::make_unique<CommentNode>("ignored"));
    body->add_child(std::make_unique<Element>("span"));
    body->add_child(std::make_unique<TextNode>("World"));
    html->add_child(std::move(body));
    doc.add_child(std::move(html));

    EXPECT_EQ(doc.text_content(), "HelloWorld");
}

TEST(DocumentTest, CharsetParsesMetaVariants) {
    Document doc("");

    auto html = std::make_unique<Element>("html");
    auto head = std::make_unique<Element>("head");

    auto meta_equiv = std::make_unique<Element>("meta");
    meta_equiv->add_attribute("http-equiv", "content-type");
    meta_equiv->add_attribute("content", "text/html; Charset=UTF-8");

    auto meta_charset = std::make_unique<Element>("meta");
    meta_charset->add_attribute("charset", "  utf-8  ");

    head->add_child(std::move(meta_charset));
    head->add_child(std::move(meta_equiv));
    html->add_child(std::move(head));
    doc.add_child(std::move(html));

    EXPECT_EQ(doc.charset(), "utf-8");
    EXPECT_EQ(doc.charset(), "utf-8");
}

TEST(DocumentTest, TitleFindsFirstTitleElement) {
    Document doc("");
    auto     html = std::make_unique<Element>("html");
    auto     head = std::make_unique<Element>("head");

    auto title = std::make_unique<Element>("title");
    title->add_child(std::make_unique<TextNode>("MyTitle"));
    head->add_child(std::move(title));

    html->add_child(std::move(head));
    doc.add_child(std::move(html));

    EXPECT_EQ(doc.title(), "MyTitle");
    EXPECT_EQ(doc.title(), "MyTitle");
}

TEST(DocumentTest, DirectLookupHelpersDoNotRequireCssEscaping) {
    Document doc("");

    auto first = std::make_unique<Element>("div");
    first->add_attribute("id", "plain");

    auto second = std::make_unique<Element>("section");
    auto target = std::make_unique<Element>("article");
    target->add_attribute("id", "a:b");
    target->add_attribute("class", "entry x:y");
    const auto* target_ptr = target.get();

    second->add_child(std::move(target));
    doc.add_child(std::move(first));
    doc.add_child(std::move(second));

    EXPECT_EQ(doc.get_element_by_id("a:b"), target_ptr);

    const auto by_class = doc.get_elements_by_class_name("x:y");
    ASSERT_EQ(by_class.size(), 1u);
    EXPECT_EQ(by_class[0], target_ptr);
}

TEST(DocumentTest, QueryIndexesInvalidateAfterAttachedDomMutations) {
    Document doc("");

    auto root = std::make_unique<Element>("div");
    auto* root_ptr = root.get();
    doc.add_child(std::move(root));

    EXPECT_EQ(doc.get_element_by_id("late-id"), nullptr);
    EXPECT_TRUE(doc.get_elements_by_tag_name("span").empty());
    EXPECT_TRUE(doc.get_elements_by_class_name("alpha").empty());

    auto child = std::make_unique<Element>("span");
    child->add_attribute("id", "late-id");
    child->add_attribute("class", "alpha beta");
    auto* child_ptr = child.get();
    root_ptr->add_child(std::move(child));

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
