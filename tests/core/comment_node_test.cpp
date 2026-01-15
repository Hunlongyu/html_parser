#include "hps/core/comment_node.hpp"
#include "hps/core/element.hpp"

#include <gtest/gtest.h>
#include <memory>

namespace hps::tests {

TEST(CommentNodeTest, BasicPropertiesAndTrim) {
    const CommentNode node("  hello  ");
    EXPECT_EQ(node.type(), NodeType::Comment);
    EXPECT_TRUE(node.is_comment());
    EXPECT_FALSE(node.is_text());
    EXPECT_FALSE(node.empty());
    EXPECT_EQ(node.length(), std::string("  hello  ").length());
    EXPECT_EQ(node.value(), "  hello  ");
    EXPECT_EQ(node.comment(), "  hello  ");
    EXPECT_EQ(node.text_content(), "  hello  ");
    EXPECT_EQ(node.trim(), "hello");
}

TEST(CommentNodeTest, ParentAndSiblingPointersAreSetByAppending) {
    auto parent = std::make_unique<Element>("div");
    auto c1     = std::make_unique<CommentNode>("a");
    auto c2     = std::make_unique<CommentNode>("b");

    const Node* c1_ptr = c1.get();
    const Node* c2_ptr = c2.get();

    parent->add_child(std::move(c1));
    parent->add_child(std::move(c2));

    EXPECT_EQ(c1_ptr->parent(), parent.get());
    EXPECT_EQ(c2_ptr->parent(), parent.get());
    EXPECT_EQ(c1_ptr->next_sibling(), c2_ptr);
    EXPECT_EQ(c2_ptr->previous_sibling(), c1_ptr);
}

}  // namespace hps::tests

