#include "hps/query/css/css_lexer.hpp"
#include <gtest/gtest.h>

namespace hps::tests {

class CSSLexerTest : public ::testing::Test {
protected:
    StringPool pool;
};

TEST_F(CSSLexerTest, BasicTokens) {
    CSSLexer lexer("div .cls #id", pool);
    
    auto t1 = lexer.next_token();
    EXPECT_EQ(t1.type, CSSLexer::CSSTokenType::Identifier);
    EXPECT_EQ(t1.value, "div");
    
    auto t2 = lexer.next_token(); // whitespace
    EXPECT_EQ(t2.type, CSSLexer::CSSTokenType::Whitespace);
    
    auto t3 = lexer.next_token();
    EXPECT_EQ(t3.type, CSSLexer::CSSTokenType::Dot);
    
    auto t4 = lexer.next_token();
    EXPECT_EQ(t4.type, CSSLexer::CSSTokenType::Identifier);
    EXPECT_EQ(t4.value, "cls");
    
    auto t5 = lexer.next_token(); // whitespace
    
    auto t6 = lexer.next_token();
    EXPECT_EQ(t6.type, CSSLexer::CSSTokenType::Hash);
    EXPECT_EQ(t6.value, "id");
    
    auto t7 = lexer.next_token();
    EXPECT_EQ(t7.type, CSSLexer::CSSTokenType::EndOfFile);
}

TEST_F(CSSLexerTest, Combinators) {
    CSSLexer lexer("> + ~", pool);
    
    EXPECT_EQ(lexer.next_token().type, CSSLexer::CSSTokenType::Greater);
    EXPECT_EQ(lexer.next_token().type, CSSLexer::CSSTokenType::Whitespace);
    EXPECT_EQ(lexer.next_token().type, CSSLexer::CSSTokenType::Plus);
    EXPECT_EQ(lexer.next_token().type, CSSLexer::CSSTokenType::Whitespace);
    EXPECT_EQ(lexer.next_token().type, CSSLexer::CSSTokenType::Tilde);
}

TEST_F(CSSLexerTest, AttributeSelectors) {
    CSSLexer lexer("[href='url']", pool);
    
    EXPECT_EQ(lexer.next_token().type, CSSLexer::CSSTokenType::LeftBracket);
    EXPECT_EQ(lexer.next_token().type, CSSLexer::CSSTokenType::Identifier); // href
    EXPECT_EQ(lexer.next_token().type, CSSLexer::CSSTokenType::Equals);
    
    auto str = lexer.next_token();
    EXPECT_EQ(str.type, CSSLexer::CSSTokenType::String);
    EXPECT_EQ(str.value, "url");
    
    EXPECT_EQ(lexer.next_token().type, CSSLexer::CSSTokenType::RightBracket);
}

TEST_F(CSSLexerTest, PeekToken) {
    CSSLexer lexer("div", pool);
    
    auto t1 = lexer.peek_token();
    EXPECT_EQ(t1.type, CSSLexer::CSSTokenType::Identifier);
    EXPECT_EQ(t1.value, "div");
    
    auto t2 = lexer.next_token();
    EXPECT_EQ(t2.type, t1.type);
    EXPECT_EQ(t2.value, t1.value);
}

} // namespace hps::tests
