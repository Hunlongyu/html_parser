#include "hps/core/comment_node.hpp"
#include "hps/core/document.hpp"
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
    EXPECT_EQ(node.text_content(), "");
    EXPECT_EQ(node.trim(), "hello");
}

TEST(CommentNodeTest, ParentAndSiblingPointersAreSetByAppending) {
    Document doc("");
    Element*     parent = doc.create_element("div");
    CommentNode* c1     = doc.create_comment("a");
    CommentNode* c2     = doc.create_comment("b");

    const Node* c1_ptr = c1;
    const Node* c2_ptr = c2;

    parent->add_child(c1);
    parent->add_child(c2);

    EXPECT_EQ(c1_ptr->parent(), parent);
    EXPECT_EQ(c2_ptr->parent(), parent);
    EXPECT_EQ(c1_ptr->next_sibling(), c2_ptr);
    EXPECT_EQ(c2_ptr->previous_sibling(), c1_ptr);
}

}  // namespace hps::tests
