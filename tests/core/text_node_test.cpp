#include "hps/core/element.hpp"
#include "hps/core/text_node.hpp"

#include <gtest/gtest.h>
#include <memory>

namespace hps::tests {

TEST(TextNodeTest, BasicPropertiesAndTrim) {
    const TextNode node("  hello  ");
    EXPECT_EQ(node.type(), NodeType::Text);
    EXPECT_TRUE(node.is_text());
    EXPECT_FALSE(node.is_comment());
    EXPECT_FALSE(node.empty());
    EXPECT_EQ(node.length(), std::string("  hello  ").length());
    EXPECT_EQ(node.value(), "  hello  ");
    EXPECT_EQ(node.text(), "  hello  ");
    EXPECT_EQ(node.text_content(), "  hello  ");
    EXPECT_EQ(node.trim(), "hello");
}

TEST(TextNodeTest, NameIsParentTagNameWhenParentIsElement) {
    auto parent = std::make_unique<Element>("p");
    auto text   = std::make_unique<TextNode>("x");
    const auto* text_ptr = text.get();

    parent->add_child(std::move(text));
    EXPECT_EQ(text_ptr->name(), "p");
}

}  // namespace hps::tests

