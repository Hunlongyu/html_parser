#include "hps/query/css/css_parser.hpp"
#include <gtest/gtest.h>

namespace hps::tests {

TEST(CSSParserTest, BasicSelector) {
    CSSParser parser("div");
    auto selector = parser.parse_selector();
    ASSERT_NE(selector, nullptr);
    EXPECT_EQ(selector->type(), SelectorType::Type);
}

TEST(CSSParserTest, ComplexSelector) {
    CSSParser parser("div.cls#id");
    auto selector = parser.parse_selector();
    ASSERT_NE(selector, nullptr);
    EXPECT_EQ(selector->type(), SelectorType::Compound);
    
    // Check specificities?
    // 1 tag, 1 class, 1 id
    auto spec = selector->calculate_specificity();
    EXPECT_EQ(spec.ids, 1);
    EXPECT_EQ(spec.classes, 1);
    EXPECT_EQ(spec.elements, 1);
}

TEST(CSSParserTest, CombinatorSelector) {
    CSSParser parser("div > p");
    auto selector = parser.parse_selector();
    ASSERT_NE(selector, nullptr);
    EXPECT_EQ(selector->type(), SelectorType::Child);
}

TEST(CSSParserTest, SelectorList) {
    CSSParser parser("div, p");
    auto list = parser.parse_selector_list();
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(list->size(), 2u);
}

TEST(CSSParserTest, InvalidSelector) {
    CSSParser parser("div[");
    auto selector = parser.parse_selector();
    // Depends on error handling mode, but here we expect validation to fail or errors to be present
    EXPECT_TRUE(parser.has_errors());
    // In lenient mode, it might return partial result or nullptr
}

TEST(CSSParserTest, Validate) {
    CSSParser parser("div.valid");
    EXPECT_TRUE(parser.validate());
    
    CSSParser invalid("div[invalid");
    EXPECT_FALSE(invalid.validate());
}

} // namespace hps::tests
