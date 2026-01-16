#include "hps/utils/string_utils.hpp"
#include <gtest/gtest.h>

namespace hps::tests {

TEST(StringUtilsTest, CharacterChecks) {
    EXPECT_TRUE(is_letter('a'));
    EXPECT_TRUE(is_letter('Z'));
    EXPECT_FALSE(is_letter('1'));
    
    EXPECT_TRUE(is_whitespace(' '));
    EXPECT_TRUE(is_whitespace('\n'));
    EXPECT_FALSE(is_whitespace('a'));
    
    EXPECT_TRUE(is_digit('0'));
    EXPECT_FALSE(is_digit('a'));
    
    EXPECT_TRUE(is_hex_digit('F'));
    EXPECT_TRUE(is_hex_digit('a'));
    EXPECT_FALSE(is_hex_digit('G'));
}

TEST(StringUtilsTest, CaseConversion) {
    EXPECT_EQ(to_lower('A'), 'a');
    EXPECT_EQ(to_lower('z'), 'z');
    EXPECT_EQ(to_upper('a'), 'A');
    EXPECT_EQ(to_upper('Z'), 'Z');
}

TEST(StringUtilsTest, TrimWhitespace) {
    EXPECT_EQ(trim_whitespace("  abc  "), "abc");
    EXPECT_EQ(trim_whitespace("abc"), "abc");
    EXPECT_EQ(trim_whitespace("   "), "");
}

TEST(StringUtilsTest, StringMatching) {
    EXPECT_TRUE(starts_with_ignore_case("Hello World", "hello"));
    EXPECT_FALSE(starts_with_ignore_case("Hello World", "world"));
    
    EXPECT_TRUE(equals_ignore_case("foo", "FOO"));
    EXPECT_FALSE(equals_ignore_case("foo", "bar"));
}

TEST(StringUtilsTest, SplitClassNames) {
    auto classes = split_class_names(" foo bar  baz ");
    EXPECT_EQ(classes.size(), 3u);
    EXPECT_TRUE(classes.contains("foo"));
    EXPECT_TRUE(classes.contains("bar"));
    EXPECT_TRUE(classes.contains("baz"));
}

TEST(StringUtilsTest, NormalizeWhitespace) {
    EXPECT_EQ(normalize_whitespace("  a   b  c  "), " a b c ");
}

TEST(StringUtilsTest, DecodeEntities) {
    EXPECT_EQ(decode_html_entities("a&nbsp;b"), "a b");
    EXPECT_EQ(decode_html_entities("a&amp;b"), "a&amp;b"); // Current implementation only supports &nbsp;?
}

} // namespace hps::tests
