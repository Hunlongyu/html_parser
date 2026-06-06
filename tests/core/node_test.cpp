#include "hps/core/comment_node.hpp"
#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/core/node.hpp"
#include "hps/core/text_node.hpp"

#include <gtest/gtest.h>
#include <memory>

namespace hps::tests {

class NodeTestHarness final : public Node {
  public:
    explicit NodeTestHarness(const NodeType type) noexcept
        : Node(type) {}

    using Node::append_child;
};

TEST(NodeTest, ParentChildrenAndSiblingPointersAreMaintained) {
    Document doc("");
    Element* parent = doc.create_element("div");

    Element*     a = doc.create_element("a");
    TextNode*    b = doc.create_text("x");
    CommentNode* c = doc.create_comment("y");

    const Node* a_ptr = a;
    const Node* b_ptr = b;
    const Node* c_ptr = c;

    parent->add_child(a);
    parent->add_child(b);
    parent->add_child(c);

    EXPECT_EQ(parent->first_child(), a_ptr);
    EXPECT_EQ(parent->last_child(), c_ptr);

    EXPECT_EQ(a_ptr->parent(), parent);
    EXPECT_EQ(b_ptr->parent(), parent);
    EXPECT_EQ(c_ptr->parent(), parent);

    EXPECT_EQ(a_ptr->previous_sibling(), nullptr);
    EXPECT_EQ(a_ptr->next_sibling(), b_ptr);
    EXPECT_EQ(b_ptr->previous_sibling(), a_ptr);
    EXPECT_EQ(b_ptr->next_sibling(), c_ptr);
    EXPECT_EQ(c_ptr->previous_sibling(), b_ptr);
    EXPECT_EQ(c_ptr->next_sibling(), nullptr);

    const auto siblings_of_b = b_ptr->siblings();
    ASSERT_EQ(siblings_of_b.size(), 2u);
    EXPECT_EQ(siblings_of_b[0], a_ptr);
    EXPECT_EQ(siblings_of_b[1], c_ptr);
}

TEST(NodeTest, EmptyNodeHasNoParentOrChildren) {
    NodeTestHarness node(NodeType::Element);

    EXPECT_EQ(node.parent(), nullptr);
    EXPECT_FALSE(node.has_parent());

    EXPECT_TRUE(node.children().empty());
    EXPECT_FALSE(node.has_children());
    EXPECT_EQ(node.first_child(), nullptr);
    EXPECT_EQ(node.last_child(), nullptr);

    EXPECT_TRUE(node.siblings().empty());

    EXPECT_EQ(node.type(), NodeType::Element);
    EXPECT_TRUE(node.is_element());
    EXPECT_FALSE(node.is_document());
    EXPECT_FALSE(node.is_text());
    EXPECT_FALSE(node.is_comment());

    EXPECT_TRUE(node.text_content().empty());

    // as_X() 采用类型标签下转（封闭层级，type 标签即实际类型）：
    // type()==Element 时 as_element 非空，其余为空。
    EXPECT_NE(node.as_element(), nullptr);
    EXPECT_EQ(node.as_document(), nullptr);
    EXPECT_EQ(node.as_text(), nullptr);
    EXPECT_EQ(node.as_comment(), nullptr);
}

TEST(NodeTest, AppendChildIgnoresNullptr) {
    NodeTestHarness parent(NodeType::Element);
    parent.append_child(nullptr);

    EXPECT_TRUE(parent.children().empty());
    EXPECT_FALSE(parent.has_children());
    EXPECT_EQ(parent.first_child(), nullptr);
    EXPECT_EQ(parent.last_child(), nullptr);
}

TEST(NodeTest, SingleChildHasNoSiblingsAndParentLinksSet) {
    NodeTestHarness parent(NodeType::Element);
    auto            child = std::make_unique<NodeTestHarness>(NodeType::Text);
    const Node*     child_ptr = child.get();

    parent.append_child(child.get());

    EXPECT_TRUE(parent.has_children());
    EXPECT_EQ(parent.first_child(), child_ptr);
    EXPECT_EQ(parent.last_child(), child_ptr);

    EXPECT_TRUE(child_ptr->has_parent());
    EXPECT_EQ(child_ptr->parent(), &parent);

    EXPECT_EQ(child_ptr->previous_sibling(), nullptr);
    EXPECT_EQ(child_ptr->next_sibling(), nullptr);
    EXPECT_TRUE(child_ptr->siblings().empty());

    const auto children = parent.children();
    ASSERT_EQ(children.size(), 1u);
    EXPECT_EQ(children[0], child_ptr);
}

TEST(NodeTest, TypePredicatesAndDynamicCastsWork) {
    Document doc("");
    Element*     html = doc.create_element("html");
    TextNode*    text = doc.create_text("t");
    CommentNode* comm = doc.create_comment("c");

    const Node* html_ptr = html;
    const Node* text_ptr = text;
    const Node* comm_ptr = comm;

    html->add_child(text);
    html->add_child(comm);
    doc.add_child(html);

    EXPECT_TRUE(doc.is_document());
    EXPECT_NE(doc.as_document(), nullptr);
    EXPECT_EQ(doc.as_element(), nullptr);

    EXPECT_TRUE(html_ptr->is_element());
    EXPECT_NE(html_ptr->as_element(), nullptr);

    EXPECT_TRUE(text_ptr->is_text());
    EXPECT_NE(text_ptr->as_text(), nullptr);
    EXPECT_EQ(text_ptr->as_comment(), nullptr);

    EXPECT_TRUE(comm_ptr->is_comment());
    EXPECT_NE(comm_ptr->as_comment(), nullptr);
    EXPECT_EQ(comm_ptr->as_text(), nullptr);
}

}  // namespace hps::tests
