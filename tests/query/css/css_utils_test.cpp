#include "hps/query/css/css_utils.hpp"
#include <gtest/gtest.h>

namespace hps::tests {

TEST(CSSUtilsTest, IsValidSelector) {
    EXPECT_TRUE(is_valid_selector("div"));
    EXPECT_TRUE(is_valid_selector("#id"));
    EXPECT_FALSE(is_valid_selector("div["));
}

TEST(CSSUtilsTest, NormalizeSelector) {
    EXPECT_EQ(normalize_selector(" DIV  >  p "), "div > p");
    EXPECT_EQ(normalize_selector(".CLASS"), ".class");
}

TEST(CSSUtilsTest, ParseCSSSelector) {
    auto list = parse_css_selector("div, p");
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(list->size(), 2u);
}

} // namespace hps::tests
