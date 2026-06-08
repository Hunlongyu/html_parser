#include "hps/core/attribute.hpp"

#include "hps/core/attr_table.hpp"

#include <gtest/gtest.h>

namespace hps::tests {

// Attribute 现在是轻量值类型：Attr id + 名/值 string_view（驻留后的稳定视图）。
// 单元测试用字符串字面量（静态存储）作为稳定视图。

TEST(AttributeTest, DefaultConstructedHasNoValue) {
    const Attribute attr;
    EXPECT_EQ(attr.id(), Attr::Unknown);
    EXPECT_EQ(attr.name(), "");
    EXPECT_EQ(attr.value(), "");
    EXPECT_FALSE(attr.has_value());
    EXPECT_EQ(attr.to_string(), "");
}

TEST(AttributeTest, ConstructedWithValueSerializes) {
    const Attribute attr(attr::kId, "id", "header", true);
    EXPECT_EQ(attr.id(), attr::kId);
    EXPECT_EQ(attr.name(), "id");
    EXPECT_EQ(attr.value(), "header");
    EXPECT_TRUE(attr.has_value());
    EXPECT_EQ(attr.to_string(), "id=\"header\"");
}

TEST(AttributeTest, ConstructedWithoutValueSerializesNameOnly) {
    const Attribute attr(attr::from_name_ci("disabled"), "disabled", "", false);
    EXPECT_EQ(attr.name(), "disabled");
    EXPECT_EQ(attr.value(), "");
    EXPECT_FALSE(attr.has_value());
    EXPECT_EQ(attr.to_string(), "disabled");
}

TEST(AttributeTest, IntegerIdResolvesForKnownAndUnknown) {
    const Attribute cls(attr::kClass, "class", "a b", true);
    EXPECT_EQ(cls.id(), attr::kClass);
    EXPECT_EQ(cls.name(), "class");
    EXPECT_EQ(cls.value(), "a b");

    const Attribute custom(Attr::Unknown, "data-x", "y", true);
    EXPECT_EQ(custom.id(), Attr::Unknown);
    EXPECT_EQ(custom.name(), "data-x");
}

TEST(AttributeTest, SetValueUpdatesValueAndFlag) {
    Attribute attr(attr::kId, "id", "x", true);
    attr.set_value("y");
    EXPECT_EQ(attr.value(), "y");
    EXPECT_TRUE(attr.has_value());

    attr.set_value("", false);
    EXPECT_EQ(attr.value(), "");
    EXPECT_FALSE(attr.has_value());
    EXPECT_EQ(attr.to_string(), "id");
}

TEST(AttributeTest, EqualityIsValueWise) {
    const Attribute a(attr::kId, "a", "1", true);
    const Attribute b(attr::kId, "a", "1", true);
    const Attribute c(attr::kId, "a", "2", true);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

}  // namespace hps::tests
