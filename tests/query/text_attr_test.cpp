#include "hps/hps.hpp"
#include "hps/core/document.hpp"
#include "hps/core/element.hpp"

#include <gtest/gtest.h>

namespace hps::tests {

// ==================== Element::attr (optional) ====================

TEST(ElementAttr, ReturnsValueWhenPresent) {
    const auto doc = hps::parse(R"(<a href="/p">x</a>)");
    const auto* a  = doc->query_selector("a");
    ASSERT_NE(a, nullptr);

    const auto href = a->attr("href");
    ASSERT_TRUE(href.has_value());
    EXPECT_EQ(*href, "/p");
}

TEST(ElementAttr, DistinguishesAbsentFromEmpty) {
    const auto doc = hps::parse(R"(<input value="">)");
    const auto* in = doc->query_selector("input");
    ASSERT_NE(in, nullptr);

    // 存在但为空 → 有值的 optional（空视图）
    const auto value = in->attr("value");
    ASSERT_TRUE(value.has_value());
    EXPECT_TRUE(value->empty());

    // 不存在 → nullopt
    EXPECT_FALSE(in->attr("missing").has_value());

    // get_attribute 无法区分二者（都返回空串）
    EXPECT_TRUE(in->get_attribute("value").empty());
    EXPECT_TRUE(in->get_attribute("missing").empty());
}

TEST(ElementAttr, ValuelessBooleanAttributeIsPresentWithEmptyValue) {
    const auto doc = hps::parse("<input disabled>");
    const auto* in = doc->query_selector("input");
    ASSERT_NE(in, nullptr);

    const auto disabled = in->attr("disabled");
    ASSERT_TRUE(disabled.has_value());
    EXPECT_TRUE(disabled->empty());
}

TEST(ElementAttr, NameIsCaseInsensitive) {
    const auto doc = hps::parse(R"(<a HREF="/p">x</a>)");
    const auto* a  = doc->query_selector("a");
    ASSERT_NE(a, nullptr);

    const auto href = a->attr("HrEf");
    ASSERT_TRUE(href.has_value());
    EXPECT_EQ(*href, "/p");
}

// ==================== ElementQuery::text (cheerio 风格) ====================

TEST(QueryText, CombinesMatchedTextRecursively) {
    const auto doc = hps::parse("<ul><li>a</li><li><b>b</b>c</li></ul>");
    EXPECT_EQ(doc->css("li").text(), "abc");  // "a" + ("b"+"c")
}

TEST(QueryText, SingleMatch) {
    const auto doc = hps::parse("<h1>Title</h1>");
    EXPECT_EQ(doc->css("h1").text(), "Title");
}

TEST(QueryText, EmptySetReturnsEmptyString) {
    const auto doc = hps::parse("<p>x</p>");
    EXPECT_EQ(doc->css("nope").text(), "");
}

// ==================== ElementQuery::attr (取首个匹配) ====================

TEST(QueryAttr, ReturnsFirstElementAttribute) {
    const auto doc = hps::parse(R"(<a class="x" href="/1">1</a><a href="/2">2</a>)");
    const auto href = doc->css("a").attr("href");
    ASSERT_TRUE(href.has_value());
    EXPECT_EQ(*href, "/1");  // 首个匹配
}

TEST(QueryAttr, EmptySetReturnsNullopt) {
    const auto doc = hps::parse("<p>x</p>");
    EXPECT_FALSE(doc->css("a").attr("href").has_value());
}

TEST(QueryAttr, FirstElementLackingAttributeReturnsNullopt) {
    const auto doc = hps::parse(R"(<a>1</a><a href="/2">2</a>)");
    EXPECT_FALSE(doc->css("a").attr("href").has_value());  // 首个 <a> 无 href
}

}  // namespace hps::tests
