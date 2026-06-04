#include "hps/parsing/html_parser.hpp"
#include "hps/parsing/options.hpp"
#include "hps/core/document.hpp"
#include "hps/core/element.hpp"

#include <gtest/gtest.h>

namespace hps::tests {

TEST(HTMLEntityDecodingTest, DefaultRawKeepsEntities) {
    HTMLParser parser;
    const auto doc = parser.parse(std::string_view("<div>Tom &amp; Jerry</div>"));

    const auto* div = doc->query_selector("div");
    ASSERT_NE(div, nullptr);
    EXPECT_EQ(div->text_content(), "Tom &amp; Jerry");
}

TEST(HTMLEntityDecodingTest, DecodeModeDecodesNbsp) {
    Options options;
    options.text_processing_mode = TextProcessingMode::Decode;

    HTMLParser parser;
    const auto doc = parser.parse(std::string_view("<div>A&nbsp;B</div>"), options);

    const auto* div = doc->query_selector("div");
    ASSERT_NE(div, nullptr);
    EXPECT_EQ(div->text_content(), "A\xC2\xA0" "B");  // &nbsp; 解码为 U+00A0（非普通空格）
}

TEST(HTMLEntityDecodingTest, DecodeModeDecodesNamedAndNumericEntities) {
    Options options;
    options.text_processing_mode = TextProcessingMode::Decode;

    HTMLParser parser;
    const auto doc = parser.parse(std::string_view("<div>&amp;&#65;&#x41;&lt;</div>"), options);

    const auto* div = doc->query_selector("div");
    ASSERT_NE(div, nullptr);
    EXPECT_EQ(div->text_content(), "&AA<");
}

TEST(HTMLEntityDecodingTest, DecodingDoesNotAffectDomStructure) {
    Options options;
    options.text_processing_mode = TextProcessingMode::Decode;

    HTMLParser parser;
    const auto doc = parser.parse(std::string_view("<div>&lt;span id=\"x\"&gt;</div>"), options);

    const auto* div = doc->query_selector("div");
    ASSERT_NE(div, nullptr);
    EXPECT_EQ(div->text_content(), "<span id=\"x\">");
    EXPECT_EQ(div->query_selector("span"), nullptr);
}

}  // namespace hps::tests
