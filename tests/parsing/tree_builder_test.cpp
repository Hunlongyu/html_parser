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

    auto body = root->first_child(); // body
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
    
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "p", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "text1")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "p", ""))); // Should close previous p
    EXPECT_TRUE(builder->process_token(Token(TokenType::TEXT, "", "text2")));
    EXPECT_TRUE(builder->finish()); // Should close last p

    // Check structure in document
    // Note: Since we didn't add html/body, they are added to document root directly
    
    auto p1 = doc->first_child();
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(p1->as_element()->tag_name(), "p");
    EXPECT_EQ(p1->first_child()->as_text()->text(), "text1");

    auto p2 = p1->next_sibling();
    ASSERT_NE(p2, nullptr);
    EXPECT_EQ(p2->as_element()->tag_name(), "p");
    EXPECT_EQ(p2->first_child()->as_text()->text(), "text2");
}

TEST_F(TreeBuilderTest, VoidElements) {
    // <div><br><img></div>
    
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "div", "")));
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "br", ""))); // Void
    EXPECT_TRUE(builder->process_token(Token(TokenType::OPEN, "img", ""))); // Void
    EXPECT_TRUE(builder->process_token(Token(TokenType::CLOSE, "div", "")));
    EXPECT_TRUE(builder->finish());

    auto div = doc->first_child();
    ASSERT_NE(div, nullptr);
    
    auto br = div->first_child();
    ASSERT_NE(br, nullptr);
    EXPECT_EQ(br->as_element()->tag_name(), "br");
    EXPECT_TRUE(br->children().empty());

    auto img = br->next_sibling();
    ASSERT_NE(img, nullptr);
    EXPECT_EQ(img->as_element()->tag_name(), "img");
    EXPECT_TRUE(img->children().empty());
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
