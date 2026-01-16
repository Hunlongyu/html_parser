#include "hps/parsing/token_attribute.hpp"
#include <gtest/gtest.h>

namespace hps::tests {

TEST(TokenAttributeTest, DefaultConstructor) {
    TokenAttribute attr;
    EXPECT_TRUE(attr.name.empty());
    EXPECT_TRUE(attr.value.empty());
    EXPECT_TRUE(attr.has_value);
}

TEST(TokenAttributeTest, ParameterizedConstructor) {
    TokenAttribute attr("href", "https://example.com");
    EXPECT_EQ(attr.name, "href");
    EXPECT_EQ(attr.value, "https://example.com");
    EXPECT_TRUE(attr.has_value);
}

TEST(TokenAttributeTest, BooleanAttribute) {
    TokenAttribute attr("checked", "", false);
    EXPECT_EQ(attr.name, "checked");
    EXPECT_TRUE(attr.value.empty());
    EXPECT_FALSE(attr.has_value);
}

TEST(TokenAttributeTest, MoveConstructor) {
    std::string name = "class";
    TokenAttribute attr(std::move(name), "foo");
    EXPECT_EQ(attr.name, "class");
    // name might be empty now, but implementation detail
}

} // namespace hps::tests
