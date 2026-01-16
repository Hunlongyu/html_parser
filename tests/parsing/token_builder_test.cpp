#include "hps/parsing/token_builder.hpp"
#include <gtest/gtest.h>

namespace hps::tests {

TEST(TokenBuilderTest, Reset) {
    TokenBuilder builder;
    builder.tag_name = "div";
    builder.attr_name = "id";
    builder.is_void_element = true;
    builder.is_self_closing = true;
    builder.add_attr("class", "foo");

    builder.reset();

    EXPECT_TRUE(builder.tag_name.empty());
    EXPECT_TRUE(builder.attr_name.empty());
    EXPECT_FALSE(builder.is_void_element);
    EXPECT_FALSE(builder.is_self_closing);
    EXPECT_TRUE(builder.attrs.empty());
}

TEST(TokenBuilderTest, AddAttribute) {
    TokenBuilder builder;
    builder.add_attr("id", "main");
    builder.add_attr("checked", "", false);

    ASSERT_EQ(builder.attrs.size(), 2u);
    EXPECT_EQ(builder.attrs[0].name, "id");
    EXPECT_EQ(builder.attrs[0].value, "main");
    EXPECT_TRUE(builder.attrs[0].has_value);

    EXPECT_EQ(builder.attrs[1].name, "checked");
    EXPECT_FALSE(builder.attrs[1].has_value);
}

TEST(TokenBuilderTest, FinishBooleanAttribute) {
    TokenBuilder builder;
    builder.attr_name = "disabled";
    builder.finish_boolean_attribute();

    ASSERT_EQ(builder.attrs.size(), 1u);
    EXPECT_EQ(builder.attrs[0].name, "disabled");
    EXPECT_FALSE(builder.attrs[0].has_value);
    EXPECT_TRUE(builder.attr_name.empty());
}

TEST(TokenBuilderTest, FinishAttribute) {
    TokenBuilder builder;
    builder.attr_name = "href";
    builder.finish_attribute("https://example.com");

    ASSERT_EQ(builder.attrs.size(), 1u);
    EXPECT_EQ(builder.attrs[0].name, "href");
    EXPECT_EQ(builder.attrs[0].value, "https://example.com");
    EXPECT_TRUE(builder.attrs[0].has_value);
    EXPECT_TRUE(builder.attr_name.empty());
}

} // namespace hps::tests
