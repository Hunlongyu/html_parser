#include "hps/parsing/tokenizer.hpp"
#include <gtest/gtest.h>

using namespace hps;

TEST(TokenizerStatesTest, EndTagNameEofDoesNotReturnToken) {
    Tokenizer tz("</div", Options());
    (void)tz.tokenize_all();
    const auto errors = tz.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::UnexpectedEOF);
}

TEST(TokenizerStatesTest, AttributeNameReturnsTagTokenOnBooleanAttributeClose) {
    Tokenizer tz("<div checked>", Options());
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type(), TokenType::OPEN);
    EXPECT_EQ(tokens[0].name(), "div");
    ASSERT_EQ(tokens[0].attrs().size(), 1u);
    EXPECT_EQ(tokens[0].attrs()[0].name, "checked");
    EXPECT_FALSE(tokens[0].attrs()[0].has_value);
}

TEST(TokenizerStatesTest, AfterAttributeNameReturnsTagTokenOnWhitespaceThenClose) {
    Tokenizer tz("<div checked >", Options());
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type(), TokenType::OPEN);
    EXPECT_EQ(tokens[0].name(), "div");
    ASSERT_EQ(tokens[0].attrs().size(), 1u);
    EXPECT_EQ(tokens[0].attrs()[0].name, "checked");
    EXPECT_FALSE(tokens[0].attrs()[0].has_value);
}

TEST(TokenizerStatesTest, BeforeAttributeValueReturnsTagTokenOnMissingValue) {
    Tokenizer tz("<div a=>", Options());
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type(), TokenType::OPEN);
    EXPECT_EQ(tokens[0].name(), "div");
    ASSERT_EQ(tokens[0].attrs().size(), 1u);
    EXPECT_EQ(tokens[0].attrs()[0].name, "a");
    EXPECT_FALSE(tokens[0].attrs()[0].has_value);

    const auto errors = tz.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::InvalidToken);
}

TEST(TokenizerStatesTest, SelfClosingStartTagBreaksOnUnexpectedCharacter) {
    Tokenizer tz("<br/ >", Options());
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type(), TokenType::CLOSE_SELF);
    EXPECT_EQ(tokens[0].name(), "br");

    const auto errors = tz.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::InvalidToken);
}

TEST(TokenizerStatesTest, DoctypeMissingGtDoesNotReturnToken) {
    Tokenizer tz("<!DOCTYPE html", Options());
    const auto tokens = tz.tokenize_all();
    EXPECT_TRUE(tokens.empty());
    const auto errors = tz.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::UnexpectedEOF);
}

TEST(TokenizerStatesTest, RawtextStyleProducesTextAndEndTagTokens) {
    Tokenizer tz("<style>abc</style>", Options());
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type(), TokenType::OPEN);
    EXPECT_EQ(tokens[0].name(), "style");
    EXPECT_EQ(tokens[1].type(), TokenType::TEXT);
    EXPECT_EQ(tokens[1].value(), "abc");
    EXPECT_EQ(tokens[2].type(), TokenType::CLOSE);
    EXPECT_EQ(tokens[2].name(), "style");
}

TEST(TokenizerStatesTest, RawtextStyleEmptyStillProducesEndTagToken) {
    Tokenizer tz("<style></style>", Options());
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type(), TokenType::OPEN);
    EXPECT_EQ(tokens[0].name(), "style");
    EXPECT_EQ(tokens[1].type(), TokenType::CLOSE);
    EXPECT_EQ(tokens[1].name(), "style");
}

TEST(TokenizerStatesTest, RcdataTextareaProducesTextAndEndTagTokens) {
    Tokenizer tz("<textarea>abc</textarea>", Options());
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type(), TokenType::OPEN);
    EXPECT_EQ(tokens[0].name(), "textarea");
    EXPECT_EQ(tokens[1].type(), TokenType::TEXT);
    EXPECT_EQ(tokens[1].value(), "abc");
    EXPECT_EQ(tokens[2].type(), TokenType::CLOSE);
    EXPECT_EQ(tokens[2].name(), "textarea");
}

TEST(TokenizerStatesTest, RcdataTextareaEmptyStillProducesEndTagToken) {
    Tokenizer tz("<textarea></textarea>", Options());
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type(), TokenType::OPEN);
    EXPECT_EQ(tokens[0].name(), "textarea");
    EXPECT_EQ(tokens[1].type(), TokenType::CLOSE);
    EXPECT_EQ(tokens[1].name(), "textarea");
}

TEST(TokenizerStatesTest, ExposesPositionLengthAndErrors) {
    Tokenizer tz("abc", Options());
    EXPECT_EQ(tz.position(), 0u);
    EXPECT_EQ(tz.total_length(), 3u);
    EXPECT_TRUE(tz.get_errors().empty());
}

TEST(TokenizerStatesTest, TagOpenQuestionMarkIsIgnored) {
    Tokenizer tz("a<?pi?>b", Options());
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type(), TokenType::TEXT);
    EXPECT_EQ(tokens[0].value(), "a");
    EXPECT_EQ(tokens[1].type(), TokenType::TEXT);
    EXPECT_EQ(tokens[1].value(), "b");
}

TEST(TokenizerStatesTest, TagOpenFallbackEmitsLessThanAsText) {
    Tokenizer tz("<>", Options());
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type(), TokenType::TEXT);
    EXPECT_EQ(tokens[0].value(), "<");
    EXPECT_EQ(tokens[1].type(), TokenType::TEXT);
    EXPECT_EQ(tokens[1].value(), ">");
}

TEST(TokenizerStatesTest, TagOpenBangUnknownGoesToCommentState) {
    Tokenizer tz("<!foo>", Options());
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type(), TokenType::COMMENT);
    EXPECT_EQ(tokens[0].value(), "foo>");
    const auto errors = tz.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::UnexpectedEOF);
}

TEST(TokenizerStatesTest, PreserveCaseTagName) {
    Options options;
    options.preserve_case = true;
    Tokenizer tz("<DIV></DIV>", options);
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type(), TokenType::OPEN);
    EXPECT_EQ(tokens[0].name(), "DIV");
    EXPECT_EQ(tokens[1].type(), TokenType::CLOSE);
    EXPECT_EQ(tokens[1].name(), "DIV");
}

TEST(TokenizerStatesTest, NonPreserveCaseLowercasesUppercaseTagName) {
    Options options;
    options.preserve_case = false;
    Tokenizer tz("<DIV></DIV>", options);
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type(), TokenType::OPEN);
    EXPECT_EQ(tokens[0].name(), "div");
    EXPECT_EQ(tokens[1].type(), TokenType::CLOSE);
    EXPECT_EQ(tokens[1].name(), "div");
}

TEST(TokenizerStatesTest, EmptyEndTagRecordsError) {
    Tokenizer tz("</>", Options());
    const auto tokens = tz.tokenize_all();
    EXPECT_TRUE(tokens.empty());
    const auto errors = tz.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::InvalidToken);
}

TEST(TokenizerStatesTest, InvalidCharacterAfterEndTagOpenRecordsError) {
    Tokenizer tz("</!>", Options());
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type(), TokenType::TEXT);
    EXPECT_EQ(tokens[0].value(), "!>");
    const auto errors = tz.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::InvalidToken);
}

TEST(TokenizerStatesTest, EndTagNameInvalidCharacterStillEmitsEndTagToken) {
    Tokenizer tz("</div!>", Options());
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type(), TokenType::CLOSE);
    EXPECT_EQ(tokens[0].name(), "div");
    const auto errors = tz.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::InvalidToken);
}

TEST(TokenizerStatesTest, BeforeAttributeNameEmbeddedNullIsTreatedAsEOF) {
    std::string html = "<div ";
    html.push_back('\0');
    Tokenizer tz(std::string_view(html.data(), html.size()), Options());
    const auto tokens = tz.tokenize_all();
    (void)tokens;
    const auto errors = tz.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::UnexpectedEOF);
}

TEST(TokenizerStatesTest, BeforeAttributeNameInvalidCharacterRecordsError) {
    Tokenizer tz("<div =>", Options());
    const auto tokens = tz.tokenize_all();
    (void)tokens;
    const auto errors = tz.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::InvalidToken);
}

TEST(TokenizerStatesTest, PreserveCaseAttributeName) {
    Options options;
    options.preserve_case = true;
    Tokenizer tz("<div ID=1>", options);
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 1u);
    ASSERT_EQ(tokens[0].attrs().size(), 1u);
    EXPECT_EQ(tokens[0].attrs()[0].name, "ID");
    EXPECT_EQ(tokens[0].attrs()[0].value, "1");
}

TEST(TokenizerStatesTest, AttributeNameSlashTransitionsToSelfClosingStartTag) {
    Tokenizer tz("<div a/>", Options());
    const auto tokens = tz.tokenize_all();
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type(), TokenType::CLOSE_SELF);
    EXPECT_EQ(tokens[0].name(), "div");
    ASSERT_EQ(tokens[0].attrs().size(), 1u);
    EXPECT_EQ(tokens[0].attrs()[0].name, "a");
}

TEST(TokenizerStatesTest, AttributeNameEmbeddedNullIsTreatedAsEOF) {
    std::string html = "<div a";
    html.push_back('\0');
    Tokenizer tz(std::string_view(html.data(), html.size()), Options());
    const auto tokens = tz.tokenize_all();
    (void)tokens;
    const auto errors = tz.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::UnexpectedEOF);
}

TEST(TokenizerStatesTest, AttributeNameUnexpectedCharacterGoesToBeforeAttributeName) {
    Tokenizer tz("<div a\"b=1>", Options());
    const auto tokens = tz.tokenize_all();
    (void)tokens;
    const auto errors = tz.consume_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].code, ErrorCode::InvalidToken);
}
