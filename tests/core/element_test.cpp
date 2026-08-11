#include "hps/core/element.hpp"
#include "hps/core/comment_node.hpp"
#include "hps/core/document.hpp"
#include "hps/core/text_node.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string_view>

namespace hps::tests {

TEST(ElementTest, TagNameAndType) {
    const Element div("div");
    EXPECT_EQ(div.type(), NodeType::Element);
    EXPECT_TRUE(div.is_element());
    EXPECT_EQ(div.tag_name(), "div");
}

TEST(ElementTest, NamespaceAccessorsExposeConfiguredNamespace) {
    const Element svg("svg", NamespaceKind::Svg);
    EXPECT_EQ(svg.namespace_kind(), NamespaceKind::Svg);
    EXPECT_EQ(svg.namespace_uri(), "http://www.w3.org/2000/svg");

    const Element math("math", NamespaceKind::MathML);
    EXPECT_EQ(math.namespace_kind(), NamespaceKind::MathML);
    EXPECT_EQ(math.namespace_uri(), "http://www.w3.org/1998/Math/MathML");
}

TEST(ElementTest, AttributesAreCaseInsensitiveForLookupAndUpdate) {
    Element el("div");
    EXPECT_FALSE(el.has_attribute("ID"));

    el.add_attribute("id", "a");
    EXPECT_TRUE(el.has_attribute("ID"));
    EXPECT_EQ(el.get_attribute("ID"), "a");
    EXPECT_EQ(el.attribute_count(), 1u);

    el.add_attribute("ID", "b");
    EXPECT_TRUE(el.has_attribute("id"));
    EXPECT_EQ(el.get_attribute("id"), "b");
    EXPECT_EQ(el.attribute_count(), 1u);
}

TEST(ElementTest, ClassHelpersWorkWithWhitespaceSeparatedTokens) {
    Element el("div");
    el.add_attribute("class", " a  b\tc \n");

    EXPECT_EQ(el.class_name(), " a  b\tc \n");
    EXPECT_TRUE(el.has_class("a"));
    EXPECT_TRUE(el.has_class("b"));
    EXPECT_TRUE(el.has_class("c"));
    EXPECT_FALSE(el.has_class("d"));

    const auto names = el.class_names();
    EXPECT_EQ(names.size(), 3u);
    const auto has = [&](std::string_view n) { return std::ranges::find(names, n) != names.end(); };
    EXPECT_TRUE(has("a"));
    EXPECT_TRUE(has("b"));
    EXPECT_TRUE(has("c"));
}

TEST(ElementTest, OwnTextOnlyIncludesDirectTextNodes) {
    Document doc("");
    Element* root = doc.create_element("div");
    root->add_child(doc.create_text("A"));

    Element* child = doc.create_element("span");
    child->add_child(doc.create_text("B"));
    root->add_child(child);

    root->add_child(doc.create_text("C"));

    EXPECT_EQ(root->own_text(), "AC");
    EXPECT_EQ(root->text_content(), "ABC");
}

TEST(ElementTest, TextContentIgnoresCommentNodes) {
    Document doc("");
    Element* root = doc.create_element("div");
    root->add_child(doc.create_text("A"));
    root->add_child(doc.create_comment("hidden"));

    Element* child = doc.create_element("span");
    child->add_child(doc.create_text("B"));
    child->add_child(doc.create_comment("nested"));
    root->add_child(child);

    root->add_child(doc.create_text("C"));

    EXPECT_EQ(root->own_text(), "AC");
    EXPECT_EQ(root->text_content(), "ABC");
}

TEST(ElementTest, BooleanAttributeSemanticsCanBeStored) {
    Element el("input");
    el.add_attribute("checked", "", false);
    el.add_attribute("value", "", true);

    ASSERT_EQ(el.attribute_count(), 2u);
    EXPECT_FALSE(el.attributes()[0].has_value());
    EXPECT_TRUE(el.attributes()[1].has_value());

    el.add_attribute("checked", "checked", true);
    ASSERT_EQ(el.attribute_count(), 2u);
    EXPECT_TRUE(el.attributes()[0].has_value());
    EXPECT_EQ(el.attributes()[0].value(), "checked");
}

TEST(ElementTest, RecursiveFindByIdSearchesDescendants) {
    Document doc("");
    Element* root = doc.create_element("div");
    Element* a = doc.create_element("a");
    Element* b = doc.create_element("b");

    b->add_attribute("id", "target");
    const auto* target_ptr = b;

    a->add_child(b);
    root->add_child(a);

    EXPECT_EQ(root->get_element_by_id("target"), target_ptr);
    EXPECT_EQ(root->get_element_by_id("missing"), nullptr);
}

TEST(ElementTest, RecursiveCollectByTagNameAndClassName) {
    Document doc("");
    Element* root = doc.create_element("div");

    Element* a = doc.create_element("p");
    a->add_attribute("class", "x");

    Element* b = doc.create_element("span");
    b->add_attribute("class", "x y");

    Element* c = doc.create_element("p");
    c->add_attribute("class", "y");

    const auto* a_ptr = a;
    const auto* b_ptr = b;
    const auto* c_ptr = c;

    root->add_child(a);
    root->add_child(b);
    root->add_child(c);

    const auto ps = root->get_elements_by_tag_name("p");
    ASSERT_EQ(ps.size(), 2u);
    EXPECT_EQ(ps[0], a_ptr);
    EXPECT_EQ(ps[1], c_ptr);

    const auto xs = root->get_elements_by_class_name("x");
    ASSERT_EQ(xs.size(), 2u);
    EXPECT_EQ(xs[0], a_ptr);
    EXPECT_EQ(xs[1], b_ptr);
}

}  // namespace hps::tests
