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
        auto div = std::make_unique<Element>("div");
        
        auto p = std::make_unique<Element>("p");
        p->add_attribute("class", "a");
        
        auto span = std::make_unique<Element>("span");
        span->add_attribute("id", "b");
        
        div->add_child(std::move(p));
        div->add_child(std::move(span));
        doc->add_child(std::move(div));
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

    auto first = std::make_unique<Element>("div");
    first->add_attribute("class", "first");

    auto second = std::make_unique<Element>("section");
    auto nested = std::make_unique<Element>("span");
    nested->add_attribute("class", "target");
    second->add_child(std::move(nested));

    fragment.add_child(std::move(first));
    fragment.add_child(std::move(second));

    CSSParser parser(".target");
    auto      selector = parser.parse_selector();
    ASSERT_NE(selector, nullptr);

    const auto results = CSSMatcher::find_all(fragment, *selector);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0]->tag_name(), "span");
}

} // namespace hps::tests
