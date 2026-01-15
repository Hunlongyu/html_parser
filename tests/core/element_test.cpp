#include "hps/core/element.hpp"
#include "hps/core/text_node.hpp"

#include <gtest/gtest.h>
#include <memory>

namespace hps::tests {

TEST(ElementTest, TagNameAndType) {
    const Element div("div");
    EXPECT_EQ(div.type(), NodeType::Element);
    EXPECT_TRUE(div.is_element());
    EXPECT_EQ(div.tag_name(), "div");
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

    const auto set = el.class_names();
    EXPECT_EQ(set.size(), 3u);
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("b"));
    EXPECT_TRUE(set.contains("c"));
}

TEST(ElementTest, OwnTextOnlyIncludesDirectTextNodes) {
    Element root("div");
    root.add_child(std::make_unique<TextNode>("A"));

    auto child = std::make_unique<Element>("span");
    child->add_child(std::make_unique<TextNode>("B"));
    root.add_child(std::move(child));

    root.add_child(std::make_unique<TextNode>("C"));

    EXPECT_EQ(root.own_text(), "AC");
    EXPECT_EQ(root.text_content(), "ABC");
}

TEST(ElementTest, RecursiveFindByIdSearchesDescendants) {
    Element root("div");
    auto    a = std::make_unique<Element>("a");
    auto    b = std::make_unique<Element>("b");

    b->add_attribute("id", "target");
    const auto* target_ptr = b.get();

    a->add_child(std::move(b));
    root.add_child(std::move(a));

    EXPECT_EQ(root.get_element_by_id("target"), target_ptr);
    EXPECT_EQ(root.get_element_by_id("missing"), nullptr);
}

TEST(ElementTest, RecursiveCollectByTagNameAndClassName) {
    Element root("div");

    auto a = std::make_unique<Element>("p");
    a->add_attribute("class", "x");

    auto b = std::make_unique<Element>("span");
    b->add_attribute("class", "x y");

    auto c = std::make_unique<Element>("p");
    c->add_attribute("class", "y");

    const auto* a_ptr = a.get();
    const auto* b_ptr = b.get();
    const auto* c_ptr = c.get();

    root.add_child(std::move(a));
    root.add_child(std::move(b));
    root.add_child(std::move(c));

    const auto ps = root.get_elements_by_tag_name("p");
    ASSERT_EQ(ps.size(), 2u);
    EXPECT_EQ(ps[0], a_ptr);
    EXPECT_EQ(ps[1], c_ptr);

    const auto xs = root.get_elements_by_class_name("x");
    ASSERT_EQ(xs.size(), 2u);
    EXPECT_EQ(xs[0], a_ptr);
    EXPECT_EQ(xs[1], b_ptr);
}

}  // namespace hps::tests

