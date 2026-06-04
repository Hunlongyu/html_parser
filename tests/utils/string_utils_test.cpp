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

TEST(StringUtilsTest, NormalizeWhitespace) {
    EXPECT_EQ(normalize_whitespace("  a   b  c  "), " a b c ");
}

TEST(StringUtilsTest, DecodeEntities) {
    EXPECT_EQ(decode_html_entities("a&nbsp;b"), "a\xC2\xA0" "b");  // U+00A0
    EXPECT_EQ(decode_html_entities("a&amp;b&lt;c&gt;"), "a&b<c>");
    EXPECT_EQ(decode_html_entities("&quot;&apos;"), "\"'");
    EXPECT_EQ(decode_html_entities("&#65;&#x41;"), "AA");
    EXPECT_EQ(decode_html_entities("&#x4E2D;"), std::string("\xE4\xB8\xAD"));
    EXPECT_EQ(decode_html_entities("&unknown;"), "&unknown;");
    // 旧式无分号 + 最长匹配 + 数值无分号 + C1→Windows-1252 重映射
    EXPECT_EQ(decode_html_entities("FOO&gtBAR"), "FOO>BAR");
    EXPECT_EQ(decode_html_entities("&notin;"), std::string("\xE2\x88\x89"));
    EXPECT_EQ(decode_html_entities("&notit;"), std::string("\xC2\xAC") + "it;");
    EXPECT_EQ(decode_html_entities("&#13"), "\r");
    EXPECT_EQ(decode_html_entities("&#128;"), std::string("\xE2\x82\xAC"));  // €
    EXPECT_EQ(decode_html_entities("&#0;"), std::string("\xEF\xBF\xBD"));    // U+FFFD
}

} // namespace hps::tests
