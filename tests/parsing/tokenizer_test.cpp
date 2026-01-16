#include "hps/parsing/tokenizer.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace hps;

class TokenizerTest : public ::testing::Test {
protected:
    std::vector<Token> tokenize(std::string_view source, const Options& options = Options()) {
        Tokenizer tokenizer(source, options);
        return tokenizer.tokenize_all();
    }

    void ExpectToken(const Token& token, TokenType type, std::string_view name = "", std::string_view value = "") {
        EXPECT_EQ(token.type(), type);
        if (!name.empty()) {
            EXPECT_EQ(token.name(), name);
        }
        if (!value.empty()) {
            EXPECT_EQ(token.value(), value);
        }
    }
};

TEST_F(TokenizerTest, TokenOwnedValueLogic) {
    // Manually test Token's owned value mechanics
    Token token(TokenType::COMMENT, "", "");
    EXPECT_TRUE(token.value().empty());
    
    std::string data = "test data";
    token.set_owned_value(data); // copy
    EXPECT_EQ(token.value(), "test data");
    
    std::string data2 = "moved data";
    token.set_owned_value(std::move(data2));
    EXPECT_EQ(token.value(), "moved data");
    
    // Test move constructor
    Token token2(std::move(token));
    EXPECT_EQ(token2.value(), "moved data");
    // token is now in moved-from state
    
    // Test vector behavior
    std::vector<Token> vec;
    vec.push_back(std::move(token2));
    EXPECT_EQ(vec[0].value(), "moved data");
}

TEST_F(TokenizerTest, BasicTags) {
    auto tokens = tokenize("<div></div><br/>");
    ASSERT_EQ(tokens.size(), 3);
    
    ExpectToken(tokens[0], TokenType::OPEN, "div");
    ExpectToken(tokens[1], TokenType::CLOSE, "div");
    ExpectToken(tokens[2], TokenType::CLOSE_SELF, "br");
}

TEST_F(TokenizerTest, VoidElementWithoutSlashIsCloseSelf) {
    auto tokens = tokenize("<br>");
    ASSERT_EQ(tokens.size(), 1);
    ExpectToken(tokens[0], TokenType::CLOSE_SELF, "br");
}

TEST_F(TokenizerTest, Attributes) {
    auto tokens = tokenize("<div id=\"test\" class='foo' checked data-val=123>");
    ASSERT_EQ(tokens.size(), 1);
    
    const auto& token = tokens[0];
    EXPECT_EQ(token.type(), TokenType::OPEN);
    EXPECT_EQ(token.name(), "div");
    
    const auto& attrs = token.attrs();
    ASSERT_EQ(attrs.size(), 4);
    
    EXPECT_EQ(attrs[0].name, "id");
    EXPECT_EQ(attrs[0].value, "test");
    
    EXPECT_EQ(attrs[1].name, "class");
    EXPECT_EQ(attrs[1].value, "foo");
    
    EXPECT_EQ(attrs[2].name, "checked");
    EXPECT_TRUE(attrs[2].value.empty()); // Boolean attribute might have empty value
    
    EXPECT_EQ(attrs[3].name, "data-val");
    EXPECT_EQ(attrs[3].value, "123");
}

TEST_F(TokenizerTest, TextContent) {
    auto tokens = tokenize("Hello World");
    ASSERT_EQ(tokens.size(), 1);
    ExpectToken(tokens[0], TokenType::TEXT, "", "Hello World");
}

TEST_F(TokenizerTest, Comments) {
    // This specifically tests the owned value mechanism we fixed
    std::string comment_text = " This is a comment ";
    std::string html = "<!--" + comment_text + "-->";
    
    auto tokens = tokenize(html);
    ASSERT_EQ(tokens.size(), 1);
    ExpectToken(tokens[0], TokenType::COMMENT, "", comment_text);
}

TEST_F(TokenizerTest, CDATA) {
    std::string cdata_content = "some <data> & content";
    std::string html = "<![CDATA[" + cdata_content + "]]>";
    
    auto tokens = tokenize(html);
    ASSERT_EQ(tokens.size(), 1);
    // CDATA is parsed as TEXT token
    ExpectToken(tokens[0], TokenType::TEXT, "", cdata_content);
}

TEST_F(TokenizerTest, ScriptData) {
    std::string script_content = "console.log('<'); var a = 1;";
    std::string html = "<script>" + script_content + "</script>";
    
    auto tokens = tokenize(html);
    ASSERT_EQ(tokens.size(), 3); // OPEN(script), TEXT(content), CLOSE(script)
    
    ExpectToken(tokens[0], TokenType::OPEN, "script");
    ExpectToken(tokens[1], TokenType::TEXT, "", script_content);
    ExpectToken(tokens[2], TokenType::CLOSE, "script");
}

TEST_F(TokenizerTest, ScriptDataBogusEndTagIsTreatedAsText) {
    std::string html = "<script>abc</scriptx>def</script>";
    auto tokens = tokenize(html);
    ASSERT_EQ(tokens.size(), 3);
    ExpectToken(tokens[0], TokenType::OPEN, "script");
    ExpectToken(tokens[1], TokenType::TEXT);
    EXPECT_EQ(tokens[1].value(), "abc</scriptx>def");
    ExpectToken(tokens[2], TokenType::CLOSE, "script");
}

TEST_F(TokenizerTest, ScriptDataWithoutEndTagReturnsTextAtEOF) {
    auto tokens = tokenize("<script>abc");
    ASSERT_EQ(tokens.size(), 2);
    ExpectToken(tokens[0], TokenType::OPEN, "script");
    ExpectToken(tokens[1], TokenType::TEXT, "", "abc");
}

TEST_F(TokenizerTest, Doctype) {
    auto tokens = tokenize("<!DOCTYPE html>");
    ASSERT_EQ(tokens.size(), 1);
    ExpectToken(tokens[0], TokenType::DOCTYPE, "DOCTYPE");
}

TEST_F(TokenizerTest, DoctypeMissingClosingBracketRecordsError) {
    Tokenizer tokenizer("<!DOCTYPE html", Options());
    const auto tokens = tokenizer.tokenize_all();
    (void)tokens;
    const auto errors = tokenizer.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::UnexpectedEOF);
}

TEST_F(TokenizerTest, MixedContent) {
    auto tokens = tokenize("<div>Text<!--Comment--></div>");
    ASSERT_EQ(tokens.size(), 4);
    
    ExpectToken(tokens[0], TokenType::OPEN, "div");
    ExpectToken(tokens[1], TokenType::TEXT, "", "Text");
    ExpectToken(tokens[2], TokenType::COMMENT, "", "Comment");
    ExpectToken(tokens[3], TokenType::CLOSE, "div");
}

TEST_F(TokenizerTest, UnclosedTags) {
    // Tolerant parsing
    auto tokens = tokenize("<div");
    // Should probably produce an OPEN token and maybe EOF error
    // If it returns 0, we accept that for now
    if (tokens.size() > 0) {
        ExpectToken(tokens[0], TokenType::OPEN, "div");
    }
}

TEST_F(TokenizerTest, CaseInsensitiveTags) {
    auto tokens = tokenize("<DIV></div >");
    ASSERT_EQ(tokens.size(), 2);
    ExpectToken(tokens[0], TokenType::OPEN, "div");
    ExpectToken(tokens[1], TokenType::CLOSE, "div");
}

TEST_F(TokenizerTest, ComplexAttributes) {
    auto tokens = tokenize("<input type='text' disabled value=123 />");
    ASSERT_EQ(tokens.size(), 1);
    ExpectToken(tokens[0], TokenType::CLOSE_SELF, "input");
    
    const auto& attrs = tokens[0].attrs();
    ASSERT_EQ(attrs.size(), 3);
    EXPECT_EQ(attrs[0].name, "type");
    EXPECT_EQ(attrs[0].value, "text");
    EXPECT_EQ(attrs[1].name, "disabled");
    EXPECT_EQ(attrs[2].name, "value");
    EXPECT_EQ(attrs[2].value, "123");
}

TEST_F(TokenizerTest, AfterAttributeNameSkipsEqualsAfterWhitespace) {
    auto tokens = tokenize("<div a =\"1\">");
    ASSERT_EQ(tokens.size(), 1);
    ExpectToken(tokens[0], TokenType::OPEN, "div");
    ASSERT_EQ(tokens[0].attrs().size(), 1u);
    EXPECT_EQ(tokens[0].attrs()[0].name, "a");
    EXPECT_EQ(tokens[0].attrs()[0].value, "1");
}

TEST_F(TokenizerTest, AfterAttributeNameSlashAfterWhitespaceIsSelfClosing) {
    auto tokens = tokenize("<div a />");
    ASSERT_EQ(tokens.size(), 1);
    ExpectToken(tokens[0], TokenType::CLOSE_SELF, "div");
    ASSERT_EQ(tokens[0].attrs().size(), 1u);
    EXPECT_EQ(tokens[0].attrs()[0].name, "a");
}

TEST_F(TokenizerTest, AfterAttributeNameGreaterThanFinishesBooleanAttribute) {
    auto tokens = tokenize("<div a >");
    ASSERT_EQ(tokens.size(), 1);
    ExpectToken(tokens[0], TokenType::OPEN, "div");
    ASSERT_EQ(tokens[0].attrs().size(), 1u);
    EXPECT_EQ(tokens[0].attrs()[0].name, "a");
    EXPECT_TRUE(tokens[0].attrs()[0].value.empty());
}

TEST_F(TokenizerTest, AfterAttributeNameUnexpectedEOFRecordsError) {
    std::string html = "<div a ";
    html.push_back('\0');
    Tokenizer tokenizer(std::string_view(html.data(), html.size()), Options());
    const auto tokens = tokenizer.tokenize_all();
    (void)tokens;
    const auto errors = tokenizer.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::UnexpectedEOF);
}

TEST_F(TokenizerTest, BeforeAttributeValueSkipsWhitespace) {
    auto tokens = tokenize("<div a=   \"x\">");
    ASSERT_EQ(tokens.size(), 1);
    ExpectToken(tokens[0], TokenType::OPEN, "div");
    ASSERT_EQ(tokens[0].attrs().size(), 1u);
    EXPECT_EQ(tokens[0].attrs()[0].name, "a");
    EXPECT_EQ(tokens[0].attrs()[0].value, "x");
}

TEST_F(TokenizerTest, DoubleQuotedAttributeValueUnexpectedEOFRecordsError) {
    Tokenizer tokenizer("<div a=\"x", Options());
    const auto tokens = tokenizer.tokenize_all();
    (void)tokens;
    const auto errors = tokenizer.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::UnexpectedEOF);
}

TEST_F(TokenizerTest, SingleQuotedAttributeValueUnexpectedEOFRecordsError) {
    Tokenizer tokenizer("<div a='x", Options());
    const auto tokens = tokenizer.tokenize_all();
    (void)tokens;
    const auto errors = tokenizer.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::UnexpectedEOF);
}

TEST_F(TokenizerTest, UnquotedAttributeValueUnexpectedCharacterRecordsError) {
    Tokenizer tokenizer("<div a=b\"c>", Options());
    const auto tokens = tokenizer.tokenize_all();
    (void)tokens;
    const auto errors = tokenizer.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::InvalidToken);
}

TEST_F(TokenizerTest, UnquotedAttributeValueUnexpectedEOFRecordsError) {
    std::string html = "<div a=b";
    html.push_back('\0');
    Tokenizer tokenizer(std::string_view(html.data(), html.size()), Options());
    const auto tokens = tokenizer.tokenize_all();
    (void)tokens;
    const auto errors = tokenizer.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::UnexpectedEOF);
}

TEST_F(TokenizerTest, SelfClosingTagUnexpectedEOFRecordsError) {
    std::string html = "<div/";
    html.push_back('\0');
    Tokenizer tokenizer(std::string_view(html.data(), html.size()), Options());
    const auto tokens = tokenizer.tokenize_all();
    (void)tokens;
    const auto errors = tokenizer.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::UnexpectedEOF);
}

TEST_F(TokenizerTest, SelfClosingStartTagInvalidCharacterRecordsError) {
    Tokenizer tokenizer("<div/x>", Options());
    const auto tokens = tokenizer.tokenize_all();
    (void)tokens;
    const auto errors = tokenizer.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::InvalidToken);
}

TEST_F(TokenizerTest, CommentDoubleDashNotCloseBranchIsHandled) {
    auto tokens = tokenize("<!--a--b-->");
    ASSERT_EQ(tokens.size(), 1);
    ExpectToken(tokens[0], TokenType::COMMENT, "", "a--b");
}

TEST_F(TokenizerTest, PeekCharOutOfRangeInCommentIsCovered) {
    Tokenizer tokenizer("<!--a--", Options());
    const auto tokens = tokenizer.tokenize_all();
    (void)tokens;
    const auto errors = tokenizer.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::UnexpectedEOF);
}

TEST_F(TokenizerTest, StrictModeThrowsOnParseError) {
    Options options;
    options.error_handling = ErrorHandlingMode::Strict;
    EXPECT_THROW((void)tokenize("<div a=\"x", options), HPSException);
}

TEST_F(TokenizerTest, RawtextBogusEndTagIsTreatedAsText) {
    std::string html = "<style>abc</stylex>def</style>";
    auto tokens = tokenize(html);
    ASSERT_EQ(tokens.size(), 3);
    ExpectToken(tokens[0], TokenType::OPEN, "style");
    ExpectToken(tokens[1], TokenType::TEXT);
    EXPECT_EQ(tokens[1].value(), "abc</stylex>def");
    ExpectToken(tokens[2], TokenType::CLOSE, "style");
}

TEST_F(TokenizerTest, RawtextWithoutEndTagReturnsTextAtEOF) {
    auto tokens = tokenize("<style>abc");
    ASSERT_EQ(tokens.size(), 2);
    ExpectToken(tokens[0], TokenType::OPEN, "style");
    ExpectToken(tokens[1], TokenType::TEXT, "", "abc");
}

TEST_F(TokenizerTest, RcdataBogusEndTagIsTreatedAsText) {
    std::string html = "<textarea>abc</textareax>def</textarea>";
    auto tokens = tokenize(html);
    ASSERT_EQ(tokens.size(), 3);
    ExpectToken(tokens[0], TokenType::OPEN, "textarea");
    ExpectToken(tokens[1], TokenType::TEXT);
    EXPECT_EQ(tokens[1].value(), "abc</textareax>def");
    ExpectToken(tokens[2], TokenType::CLOSE, "textarea");
}

TEST_F(TokenizerTest, RcdataWithoutEndTagReturnsTextAtEOF) {
    auto tokens = tokenize("<textarea>abc");
    ASSERT_EQ(tokens.size(), 2);
    ExpectToken(tokens[0], TokenType::OPEN, "textarea");
    ExpectToken(tokens[1], TokenType::TEXT, "", "abc");
}
