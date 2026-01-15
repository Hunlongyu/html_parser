#include "hps/core/attribute.hpp"

#include <gtest/gtest.h>

namespace hps::tests {

TEST(AttributeTest, DefaultConstructedHasNoValue) {
    const Attribute attr;
    EXPECT_EQ(attr.name(), "");
    EXPECT_EQ(attr.value(), "");
    EXPECT_FALSE(attr.has_value());
    EXPECT_EQ(attr.to_string(), "");
}

TEST(AttributeTest, ConstructedWithValueSerializes) {
    const Attribute attr("id", "header");
    EXPECT_EQ(attr.name(), "id");
    EXPECT_EQ(attr.value(), "header");
    EXPECT_TRUE(attr.has_value());
    EXPECT_EQ(attr.to_string(), "id=\"header\"");
}

TEST(AttributeTest, ConstructedWithoutValueSerializesNameOnly) {
    const Attribute attr("disabled", "", false);
    EXPECT_EQ(attr.name(), "disabled");
    EXPECT_EQ(attr.value(), "");
    EXPECT_FALSE(attr.has_value());
    EXPECT_EQ(attr.to_string(), "disabled");
}

TEST(AttributeTest, ConstructFromTokenAttributeCopiesFields) {
    const TokenAttribute tok("class", "a b", true);
    const Attribute      attr(tok);
    EXPECT_EQ(attr.name(), "class");
    EXPECT_EQ(attr.value(), "a b");
    EXPECT_TRUE(attr.has_value());
}

TEST(AttributeTest, SettersUpdateNameAndValue) {
    Attribute attr("id", "x");
    attr.set_name(std::string_view("data-x"));
    attr.set_value(std::string_view("y"));
    EXPECT_EQ(attr.name(), "data-x");
    EXPECT_EQ(attr.value(), "y");
    EXPECT_TRUE(attr.has_value());

    attr.set_value(std::string_view(""), false);
    EXPECT_EQ(attr.value(), "");
    EXPECT_FALSE(attr.has_value());
    EXPECT_EQ(attr.to_string(), "data-x");
}

TEST(AttributeTest, ComparisonsAreLexicographicByNameValueHasValue) {
    const Attribute a("a", "1", true);
    const Attribute b("a", "1", true);
    const Attribute c("a", "2", true);
    const Attribute d("b", "1", true);
    const Attribute e("a", "1", false);

    EXPECT_EQ(a, b);
    EXPECT_LT(a, c);
    EXPECT_LT(c, d);
    EXPECT_LT(e, a);
}

}  // namespace hps::tests
