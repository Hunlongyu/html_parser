#include "hps/parsing/tree_builder.hpp"
#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/core/text_node.hpp"
#include "hps/core/comment_node.hpp"
#include "hps/parsing/token.hpp"
#include <gtest/gtest.h>
#include <memory>

namespace hps::tests {

class TreeBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        doc = std::make_shared<Document>("");
        options = Options::strict();
        builder = std::make_unique<TreeBuilder>(doc, options);
    }

    std::shared_ptr<Document> doc;
    Options options;
    std::unique_ptr<TreeBuilder> builder;
};

TEST_F(TreeBuilderTest, BasicStructure) {
    // <html><body><p>text</p></body></html>
    
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "html", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "body", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "p", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "text")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "p", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "body", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "html", "")));
    EXPECT_TRUE(builder->finish());

    auto root = doc->first_child(); // html
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->as_element()->tag_name(), "html");

    auto head = root->first_child(); // head
    ASSERT_NE(head, nullptr);
    EXPECT_EQ(head->as_element()->tag_name(), "head");

    auto body = head->next_sibling(); // body
    ASSERT_NE(body, nullptr);
    EXPECT_EQ(body->as_element()->tag_name(), "body");

    auto p = body->first_child(); // p
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->as_element()->tag_name(), "p");

    auto text = p->first_child(); // text
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->as_text()->text(), "text");
}

TEST_F(TreeBuilderTest, ImplicitClose) {
    // <p>text1<p>text2 -> <p>text1</p><p>text2</p>
    options.error_handling = ErrorHandlingMode::Lenient;
    builder = std::make_unique<TreeBuilder>(doc, options);
    
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "p", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "text1")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "p", ""))); // Should close previous p
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "text2")));
    EXPECT_TRUE(builder->finish()); // Should close last p

    const auto* html = doc->html();
    ASSERT_NE(html, nullptr);

    const auto* body = html->querySelector("body");
    ASSERT_NE(body, nullptr);

    auto p1 = body->first_child();
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(p1->as_element()->tag_name(), "p");
    EXPECT_EQ(p1->first_child()->as_text()->text(), "text1");

    auto p2 = p1->next_sibling();
    ASSERT_NE(p2, nullptr);
    EXPECT_EQ(p2->as_element()->tag_name(), "p");
    EXPECT_EQ(p2->first_child()->as_text()->text(), "text2");
}

TEST_F(TreeBuilderTest, AutoInsertsHtmlHeadAndBodyForBodyContent) {
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "div", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "hello")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "div", "")));
    EXPECT_TRUE(builder->finish());

    const auto* html = doc->html();
    ASSERT_NE(html, nullptr);

    const auto* head = html->querySelector("head");
    const auto* body = html->querySelector("body");
    ASSERT_NE(head, nullptr);
    ASSERT_NE(body, nullptr);

    const auto* div = body->querySelector("div");
    ASSERT_NE(div, nullptr);
    EXPECT_EQ(div->text_content(), "hello");
}

TEST_F(TreeBuilderTest, HeadContentBeforeBodyIsRoutedIntoHead) {
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "meta", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "title", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "Example")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "title", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "p", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "Body")));
    EXPECT_TRUE(builder->finish());

    const auto* html = doc->html();
    ASSERT_NE(html, nullptr);

    const auto* head = html->querySelector("head");
    const auto* body = html->querySelector("body");
    ASSERT_NE(head, nullptr);
    ASSERT_NE(body, nullptr);

    ASSERT_NE(head->querySelector("meta"), nullptr);
    const auto* title = head->querySelector("title");
    ASSERT_NE(title, nullptr);
    EXPECT_EQ(title->text_content(), "Example");

    const auto* paragraph = body->querySelector("p");
    ASSERT_NE(paragraph, nullptr);
    EXPECT_EQ(paragraph->text_content(), "Body");
}

TEST_F(TreeBuilderTest, VoidElements) {
    // <div><br><img></div>
    
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "div", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "br", ""))); // Void
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "img", ""))); // Void
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "div", "")));
    EXPECT_TRUE(builder->finish());

    const auto* html = doc->html();
    ASSERT_NE(html, nullptr);

    const auto* body = html->querySelector("body");
    ASSERT_NE(body, nullptr);

    auto div = body->first_child();
    ASSERT_NE(div, nullptr);
    EXPECT_EQ(div->as_element()->tag_name(), "div");
    
    auto br = div->first_child();
    ASSERT_NE(br, nullptr);
    EXPECT_EQ(br->as_element()->tag_name(), "br");
    EXPECT_TRUE(br->children().empty());

    auto img = br->next_sibling();
    ASSERT_NE(img, nullptr);
    EXPECT_EQ(img->as_element()->tag_name(), "img");
    EXPECT_TRUE(img->children().empty());
}

TEST_F(TreeBuilderTest, TableRowsAutoInsertTbody) {
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "table", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "tr", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "td", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "x")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "table", "")));
    EXPECT_TRUE(builder->finish());

    const auto* table = doc->querySelector("table");
    ASSERT_NE(table, nullptr);

    const auto* tbody = table->querySelector("tbody");
    ASSERT_NE(tbody, nullptr);

    const auto* row = tbody->querySelector("tr");
    ASSERT_NE(row, nullptr);

    const auto* cell = row->querySelector("td");
    ASSERT_NE(cell, nullptr);
    EXPECT_EQ(cell->text_content(), "x");
}

TEST_F(TreeBuilderTest, TableCellsAutoInsertRowAndClosePreviousCell) {
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "table", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "td", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "a")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "td", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "b")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "table", "")));
    EXPECT_TRUE(builder->finish());

    const auto* row = doc->querySelector("table tbody tr");
    ASSERT_NE(row, nullptr);

    const auto cells = row->querySelectorAll("td");
    ASSERT_EQ(cells.size(), 2u);
    EXPECT_EQ(cells[0]->text_content(), "a");
    EXPECT_EQ(cells[1]->text_content(), "b");
}

TEST_F(TreeBuilderTest, SelectOptionsImplicitlyClosePreviousOption) {
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "select", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "option", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "a")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "option", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "b")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "select", "")));
    EXPECT_TRUE(builder->finish());

    const auto* select = doc->querySelector("select");
    ASSERT_NE(select, nullptr);

    const auto children = select->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "option");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "option");
    EXPECT_EQ(children[0]->as_element()->text_content(), "a");
    EXPECT_EQ(children[1]->as_element()->text_content(), "b");
}

TEST_F(TreeBuilderTest, SelectOptgroupClosesOpenOptionAndPreviousOptgroup) {
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "select", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "optgroup", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "option", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "a")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "optgroup", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "option", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "b")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "select", "")));
    EXPECT_TRUE(builder->finish());

    const auto* select = doc->querySelector("select");
    ASSERT_NE(select, nullptr);

    const auto children = select->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "optgroup");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "optgroup");
    EXPECT_EQ(children[0]->as_element()->children().size(), 1u);
    EXPECT_EQ(children[1]->as_element()->children().size(), 1u);
}

TEST_F(TreeBuilderTest, ButtonStartTagImplicitlyClosesOpenButton) {
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "div", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "button", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "one")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "button", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "two")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "button", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "div", "")));
    EXPECT_TRUE(builder->finish());

    const auto* div = doc->querySelector("div");
    ASSERT_NE(div, nullptr);

    const auto children = div->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "button");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "button");
    EXPECT_EQ(children[0]->as_element()->text_content(), "one");
    EXPECT_EQ(children[1]->as_element()->text_content(), "two");
}

TEST_F(TreeBuilderTest, DuplicateAnchorStartTagClosesPreviousAnchor) {
    options.error_handling = ErrorHandlingMode::Lenient;
    builder = std::make_unique<TreeBuilder>(doc, options);

    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "div", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "a", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "one")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "a", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "two")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "a", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "div", "")));
    EXPECT_TRUE(builder->finish());

    const auto* div = doc->querySelector("div");
    ASSERT_NE(div, nullptr);

    const auto children = div->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "a");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "a");
    EXPECT_EQ(children[0]->as_element()->text_content(), "one");
    EXPECT_EQ(children[1]->as_element()->text_content(), "two");
    EXPECT_FALSE(builder->errors().empty());
}

TEST_F(TreeBuilderTest, NestedFormStartTagIsIgnored) {
    options.error_handling = ErrorHandlingMode::Lenient;
    builder = std::make_unique<TreeBuilder>(doc, options);

    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "div", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "form", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "input", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "form", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "input", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "form", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "div", "")));
    EXPECT_TRUE(builder->finish());

    const auto forms = doc->querySelectorAll("form");
    ASSERT_EQ(forms.size(), 1u);
    EXPECT_EQ(forms[0]->querySelectorAll("input").size(), 2u);
    EXPECT_FALSE(builder->errors().empty());
}

TEST_F(TreeBuilderTest, TableCaptionClosesBeforeBodyRows) {
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "table", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "caption", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "cap")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "tr", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "td", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "x")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "table", "")));
    EXPECT_TRUE(builder->finish());

    const auto* table = doc->querySelector("table");
    ASSERT_NE(table, nullptr);

    const auto children = table->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "caption");
    EXPECT_EQ(children[0]->as_element()->text_content(), "cap");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "tbody");
}

TEST_F(TreeBuilderTest, NestedTableInsideCellRemainsInsideCell) {
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "table", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "tr", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "td", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "a")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "table", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "tr", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "td", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "b")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "td", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "tr", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "table", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "c")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "td", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "tr", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "table", "")));
    EXPECT_TRUE(builder->finish());

    const auto* outer_table = doc->querySelector("table");
    ASSERT_NE(outer_table, nullptr);

    const auto* outer_row = outer_table->querySelector("tbody > tr");
    ASSERT_NE(outer_row, nullptr);

    const auto row_children = outer_row->children();
    ASSERT_EQ(row_children.size(), 1u);
    ASSERT_TRUE(row_children[0]->is_element());
    EXPECT_EQ(row_children[0]->as_element()->tag_name(), "td");

    const auto* outer_cell = row_children[0]->as_element();
    const auto children = outer_cell->children();
    ASSERT_EQ(children.size(), 3u);
    ASSERT_TRUE(children[0]->is_text());
    ASSERT_TRUE(children[1]->is_element());
    ASSERT_TRUE(children[2]->is_text());
    EXPECT_EQ(children[0]->as_text()->value(), "a");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "table");
    EXPECT_EQ(children[2]->as_text()->value(), "c");
}

TEST_F(TreeBuilderTest, TableColImpliesColgroup) {
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "table", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "col", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "col", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "table", "")));
    EXPECT_TRUE(builder->finish());

    const auto* table = doc->querySelector("table");
    ASSERT_NE(table, nullptr);

    const auto children = table->children();
    ASSERT_EQ(children.size(), 1u);
    ASSERT_TRUE(children[0]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "colgroup");

    const auto colgroup_children = children[0]->children();
    ASSERT_EQ(colgroup_children.size(), 2u);
    ASSERT_TRUE(colgroup_children[0]->is_element());
    ASSERT_TRUE(colgroup_children[1]->is_element());
    EXPECT_EQ(colgroup_children[0]->as_element()->tag_name(), "col");
    EXPECT_EQ(colgroup_children[1]->as_element()->tag_name(), "col");
}

TEST_F(TreeBuilderTest, ColgroupFallbackReprocessesNonColTokensInTableMode) {
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "section", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "table", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "colgroup", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "div", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "x")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "div", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "tr", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "td", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "y")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "table", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "section", "")));
    EXPECT_TRUE(builder->finish());

    const auto* section = doc->querySelector("section");
    ASSERT_NE(section, nullptr);

    const auto children = section->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "div");
    EXPECT_EQ(children[0]->as_element()->text_content(), "x");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "table");
}

TEST_F(TreeBuilderTest, TableTextIsFosterParentedBeforeTable) {
    options.error_handling = ErrorHandlingMode::Lenient;
    builder = std::make_unique<TreeBuilder>(doc, options);

    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "div", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "table", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "alpha")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "tr", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "td", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "beta")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "table", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "div", "")));
    EXPECT_TRUE(builder->finish());

    const auto* div = doc->querySelector("div");
    ASSERT_NE(div, nullptr);

    const auto children = div->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_text());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_text()->value(), "alpha");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "table");
}

TEST_F(TreeBuilderTest, TableElementIsFosterParentedBeforeTable) {
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "div", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "table", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "b", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "alpha")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "tr", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "td", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "beta")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "table", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "div", "")));
    EXPECT_TRUE(builder->finish());

    const auto* div = doc->querySelector("div");
    ASSERT_NE(div, nullptr);

    const auto children = div->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "b");
    EXPECT_EQ(children[0]->as_element()->text_content(), "alpha");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "table");
}

TEST_F(TreeBuilderTest, RecoversMisnestedFormattingElements) {
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "p", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "b", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "i", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "x")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "b", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "y")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "i", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "z")));
    EXPECT_TRUE(builder->finish());

    const auto* paragraph = doc->querySelector("p");
    ASSERT_NE(paragraph, nullptr);

    const auto children = paragraph->children();
    ASSERT_EQ(children.size(), 3u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    ASSERT_TRUE(children[2]->is_text());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "b");
    EXPECT_EQ(children[0]->as_element()->text_content(), "x");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "i");
    EXPECT_EQ(children[1]->as_element()->text_content(), "y");
    EXPECT_EQ(children[2]->as_text()->value(), "z");
}

TEST_F(TreeBuilderTest, Errors) {
    // </div> mismatch
    // Changing to Lenient for error collection test
    options.error_handling = ErrorHandlingMode::Lenient;
    // Re-create builder with lenient options
    builder = std::make_unique<TreeBuilder>(doc, options);
    
    // In Lenient mode, process_token returns true even on error
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "div", "")));
    
    const auto& errors = builder->errors();
    EXPECT_FALSE(errors.empty());
}

} // namespace hps::tests
