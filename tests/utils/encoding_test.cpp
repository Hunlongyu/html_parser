#include "hps/hps.hpp"

#include <string>

#include <gtest/gtest.h>

namespace hps::tests {
namespace {

auto utf16le_with_bom(std::u16string_view text) -> std::string {
    std::string bytes("\xFF\xFE", 2);
    bytes.reserve(bytes.size() + text.size() * 2);
    for (const char16_t code_unit : text) {
        bytes.push_back(static_cast<char>(code_unit & 0xFF));
        bytes.push_back(static_cast<char>((code_unit >> 8) & 0xFF));
    }
    return bytes;
}

}  // namespace

TEST(EncodingTest, SniffUtf8Bom) {
    const auto hint = sniff_html_encoding(std::string("\xEF\xBB\xBF", 3) + "<div>x</div>");
    EXPECT_EQ(hint.source, EncodingHintSource::Utf8Bom);
    EXPECT_EQ(hint.canonical_label, "utf-8");
    EXPECT_TRUE(hint.helper_supported);
}

TEST(EncodingTest, SniffMetaCharsetReportsSupportedHelperEncoding) {
    std::string bytes = "<html><head><meta charset='gbk'></head><body>";
    bytes.append("\xD6\xD0\xCE\xC4", 4);
    bytes += "</body></html>";

    const auto hint = sniff_html_encoding(bytes);
    EXPECT_EQ(hint.source, EncodingHintSource::MetaCharset);
    EXPECT_EQ(hint.canonical_label, "gbk");
    EXPECT_TRUE(hint.helper_supported);
}

TEST(EncodingTest, SniffMetaCharsetReportsUnsupportedHelperEncoding) {
    const auto hint = sniff_html_encoding("<meta charset='euc-jp'><p>x</p>");
    EXPECT_EQ(hint.source, EncodingHintSource::MetaCharset);
    EXPECT_EQ(hint.canonical_label, "euc-jp");
    EXPECT_FALSE(hint.helper_supported);
}

TEST(EncodingTest, DecodeUtf8StripsBomAndValidatesInput) {
    const auto decoded = decode_html_bytes_to_utf8(std::string("\xEF\xBB\xBF", 3) + "<div>x</div>", "utf-8");
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(*decoded, "<div>x</div>");

    EXPECT_FALSE(decode_html_bytes_to_utf8(std::string("\xFF\xFF", 2), "utf-8").has_value());
}

TEST(EncodingTest, DecodeUtf16LeHelperConvertsToUtf8) {
    const auto decoded = decode_html_bytes_to_utf8(utf16le_with_bom(u"<div>Hello</div>"), "utf-16le");
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(*decoded, "<div>Hello</div>");
}

TEST(EncodingTest, DecodeGbkHelperConvertsToUtf8) {
    std::string bytes;
    bytes.append("\xD6\xD0\xCE\xC4", 4);

    const auto decoded = decode_html_bytes_to_utf8(bytes, "gb18030");
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(*decoded, "中文");
}

TEST(EncodingTest, DecodeWindows1252HelperConvertsToUtf8) {
    std::string bytes(1, static_cast<char>(0x80));

    const auto decoded = decode_html_bytes_to_utf8(bytes, "iso-8859-1");
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(*decoded, "€");
}

TEST(EncodingTest, DecodeShiftJisHelperConvertsToUtf8) {
    std::string bytes;
    bytes.append("\x93\xFA\x96\x7B", 4);

    const auto decoded = decode_html_bytes_to_utf8(bytes, "windows-31j");
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(*decoded, "日本");
}

TEST(EncodingTest, DecodeUnsupportedEncodingReturnsNullopt) {
    EXPECT_FALSE(decode_html_bytes_to_utf8("abc", "euc-jp").has_value());
}

}  // namespace hps::tests
