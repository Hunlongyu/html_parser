#include "hps/query/css/css_matcher.hpp"
#include "hps/core/element.hpp"
#include "hps/core/document.hpp"
#include "hps/query/css/css_parser.hpp"
#include <gtest/gtest.h>

namespace hps::tests {

class CSSMatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        // <div>
        //   <p class="a">text</p>
        //   <span id="b">text</span>
        // </div>
        doc = std::make_shared<Document>("");
        Element* div = doc->create_element("div");

        Element* p = doc->create_element("p");
        p->add_attribute("class", "a");

        Element* span = doc->create_element("span");
        span->add_attribute("id", "b");

        div->add_child(p);
        div->add_child(span);
        doc->add_child(div);
    }

    std::shared_ptr<Document> doc;
};

TEST_F(CSSMatcherTest, FindAll) {
    CSSParser parser("p.a");
    auto selector = parser.parse_selector();
    ASSERT_NE(selector, nullptr);
    
    auto results = CSSMatcher::find_all(*doc, *selector);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0]->tag_name(), "p");
}

TEST_F(CSSMatcherTest, FindFirst) {
    CSSParser parser("span#b");
    auto selector = parser.parse_selector();
    ASSERT_NE(selector, nullptr);
    
    auto result = CSSMatcher::find_first(*doc, *selector);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->tag_name(), "span");
}

TEST_F(CSSMatcherTest, NoMatch) {
    CSSParser parser("div.nonexistent");
    auto selector = parser.parse_selector();
    
    auto results = CSSMatcher::find_all(*doc, *selector);
    EXPECT_TRUE(results.empty());
}

TEST(CSSMatcherFragmentTest, DocumentTraversalIncludesAllTopLevelElementSubtrees) {
    Document fragment("");

    Element* first = fragment.create_element("div");
    first->add_attribute("class", "first");

    Element* second = fragment.create_element("section");
    Element* nested = fragment.create_element("span");
    nested->add_attribute("class", "target");
    second->add_child(nested);

    fragment.add_child(first);
    fragment.add_child(second);

    CSSParser parser(".target");
    auto      selector = parser.parse_selector();
    ASSERT_NE(selector, nullptr);

    const auto results = CSSMatcher::find_all(fragment, *selector);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0]->tag_name(), "span");
}

} // namespace hps::tests
