#include "hps/parsing/token.hpp"
#include <gtest/gtest.h>

namespace hps::tests {

TEST(TokenTest, ConstructorAndAccessors) {
    Token token(TokenType::OPEN, "div", "");
    EXPECT_EQ(token.type(), TokenType::OPEN);
    EXPECT_EQ(token.name(), "div");
    EXPECT_TRUE(token.value().empty());
}

TEST(TokenTest, TypeChecks) {
    Token start(TokenType::OPEN, "div", "");
    EXPECT_TRUE(start.is_open());
    EXPECT_FALSE(start.is_close());
    EXPECT_TRUE(start.is_tag("div"));

    Token end(TokenType::CLOSE, "div", "");
    EXPECT_TRUE(end.is_close());
    EXPECT_FALSE(end.is_open());

    Token self_close(TokenType::CLOSE_SELF, "img", "");
    EXPECT_TRUE(self_close.is_close_self());

    Token text(TokenType::TEXT, "", "hello");
    EXPECT_TRUE(text.is_text());
    EXPECT_EQ(text.value(), "hello");

    Token comment(TokenType::COMMENT, "", "comment");
    EXPECT_TRUE(comment.is_comment());

    Token doctype(TokenType::DOCTYPE, "html", "");
    EXPECT_TRUE(doctype.is_doctype());
    
    Token eof(TokenType::DONE, "", "");
    EXPECT_TRUE(eof.is_done());
}

TEST(TokenTest, Attributes) {
    Token token(TokenType::OPEN, "div", "");
    
    token.add_attr("id", "main");
    token.add_attr("class", "container");
    token.add_attr("disabled", "", false); // Boolean attribute

    const auto& attrs = token.attrs();
    ASSERT_EQ(attrs.size(), 3u);
    
    EXPECT_EQ(attrs[0].name, "id");
    EXPECT_EQ(attrs[0].value, "main");
    EXPECT_TRUE(attrs[0].has_value);

    EXPECT_EQ(attrs[1].name, "class");
    EXPECT_EQ(attrs[1].value, "container");
    EXPECT_TRUE(attrs[1].has_value);

    EXPECT_EQ(attrs[2].name, "disabled");
    EXPECT_TRUE(attrs[2].value.empty());
    EXPECT_FALSE(attrs[2].has_value);
}

TEST(TokenTest, OwnedValue) {
    Token token(TokenType::TEXT, "", "");
    std::string dynamic_content = "dynamic content";
    token.set_owned_value(dynamic_content);
    
    EXPECT_EQ(token.value(), "dynamic content");
    // Ensure it's a copy/owned
    dynamic_content = "changed";
    EXPECT_EQ(token.value(), "dynamic content");
}

TEST(TokenTest, SetType) {
    Token token(TokenType::OPEN, "br", "");
    token.set_type(TokenType::CLOSE_SELF);
    EXPECT_EQ(token.type(), TokenType::CLOSE_SELF);
    EXPECT_TRUE(token.is_close_self());
}

} // namespace hps::tests
