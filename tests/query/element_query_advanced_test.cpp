#include "hps/hps.hpp"

#include "hps/query/element_query.hpp"

#include <gtest/gtest.h>

namespace hps::tests {

TEST(ElementQueryAdvancedTest, FiltersAndNavigationWorkOnParsedDom) {
    const auto doc = hps::parse(R"(
        <html><body>
          <div id="root">
            <p class="a">Hello <span class="b">World</span></p>
            <p class="a x" data-k="v1">Foo</p>
            <p class="c" data-k="v2">Bar</p>
          </div>
        </body></html>
    )");
    ASSERT_NE(doc, nullptr);

    const auto root = doc->css("#root");
    ASSERT_EQ(root.size(), 1u);

    const auto ps = root.children("p");
    ASSERT_EQ(ps.size(), 3u);

    EXPECT_EQ(ps.has_attribute("data-k").size(), 2u);
    EXPECT_EQ(ps.has_attribute("data-k", "v2").size(), 1u);
    EXPECT_EQ(ps.has_class("a").size(), 2u);
    EXPECT_EQ(ps.has_tag("p").size(), 3u);

    EXPECT_EQ(ps.first(2).size(), 2u);
    EXPECT_EQ(ps.last(2).size(), 2u);
    EXPECT_EQ(ps.skip(1).size(), 2u);
    EXPECT_EQ(ps.limit(1).size(), 1u);

    EXPECT_EQ(ps.even().size(), 2u);
    EXPECT_EQ(ps.odd().size(), 1u);
    EXPECT_EQ(ps.eq(0).size(), 1u);
    EXPECT_EQ(ps.eq(42).size(), 0u);
    EXPECT_EQ(ps.gt(0).size(), 2u);
    EXPECT_EQ(ps.lt(2).size(), 2u);

    EXPECT_EQ(ps.not_(".a").size(), 1u);

    const auto span = root.children("p").children("span");
    ASSERT_EQ(span.size(), 1u);

    EXPECT_EQ(span.parent().size(), 1u);
    EXPECT_EQ(span.closest(".a").size(), 1u);
    EXPECT_GE(span.parents().size(), 2u);

    EXPECT_EQ(ps.first(1).next_sibling().size(), 1u);
    EXPECT_EQ(ps.last(1).prev_sibling().size(), 1u);
    EXPECT_EQ(ps.eq(1).siblings().size(), 2u);

    EXPECT_EQ(root.css("p.a").size(), 2u);

    EXPECT_TRUE(ps.is("p"));
    EXPECT_TRUE(ps.is(".a"));
    EXPECT_FALSE(ps.is(""));

    EXPECT_TRUE(ps.contains("Foo"));

    const auto attrs = ps.extract_attributes("data-k");
    EXPECT_EQ(attrs.size(), 2u);

    const auto texts = ps.extract_texts();
    EXPECT_EQ(texts.size(), 3u);

    const auto own_texts = ps.extract_own_texts();
    ASSERT_EQ(own_texts.size(), 3u);
    EXPECT_NE(own_texts[0].find("Hello"), std::string::npos);

    size_t seen = 0;
    ps.each([&](size_t, const Element&) { ++seen; });
    EXPECT_EQ(seen, 3u);
}

TEST(ElementQueryAdvancedTest, PredicateBasedFiltersWork) {
    const auto doc = hps::parse(R"(
        <html><body>
          <div><p data-k="abc">one</p><p data-k="xyz">two</p></div>
        </body></html>
    )");
    ASSERT_NE(doc, nullptr);

    const auto ps = doc->css("p");
    ASSERT_EQ(ps.size(), 2u);

    const auto matched = ps.matching_text([](const std::string_view t) { return t.find("two") != std::string_view::npos; });
    EXPECT_EQ(matched.size(), 1u);

    const auto attr_contains = ps.has_attribute_contains("data-k", "b");
    EXPECT_EQ(attr_contains.size(), 1u);

    const auto text_contains = ps.has_text_contains("wo");
    EXPECT_EQ(text_contains.size(), 1u);
}

}  // namespace hps::tests

