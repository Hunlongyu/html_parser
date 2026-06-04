#include "hps/hps.hpp"
#include "hps/core/document.hpp"

#include <gtest/gtest.h>

namespace hps::tests {

// 属性选择器各运算符的匹配语义。
TEST(AttributeSelector, OperatorsMatchExpected) {
    const auto doc = hps::parse(
        "<a class='btn primary' data-x='hello-world' lang='en-US'>1</a>"
        "<b class=''>2</b>");

    EXPECT_EQ(doc->css("[class]").size(), 2u);                 // 存在性
    EXPECT_EQ(doc->css("[class='btn primary']").size(), 1u);  // 全等
    EXPECT_EQ(doc->css("[class~='primary']").size(), 1u);     // 词匹配
    EXPECT_EQ(doc->css("[data-x^='hello']").size(), 1u);      // 前缀
    EXPECT_EQ(doc->css("[data-x$='world']").size(), 1u);      // 后缀
    EXPECT_EQ(doc->css("[data-x*='lo-wo']").size(), 1u);      // 子串
    EXPECT_EQ(doc->css("[lang|='en']").size(), 1u);           // 语言段（en-US）
}

// 规范：*= / ^= / $= / ~= 的目标值为空时永不匹配；存在性 [attr] 不受影响。
TEST(AttributeSelector, EmptyTargetValueNeverMatches) {
    const auto doc = hps::parse("<a class='x'>1</a><b class=''>2</b>");

    EXPECT_TRUE(doc->css("[class*='']").empty());
    EXPECT_TRUE(doc->css("[class^='']").empty());
    EXPECT_TRUE(doc->css("[class$='']").empty());
    EXPECT_TRUE(doc->css("[class~='']").empty());
    EXPECT_EQ(doc->css("[class]").size(), 2u);
}

}  // namespace hps::tests
