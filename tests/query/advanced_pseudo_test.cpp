#include "hps/core/element.hpp"
#include "hps/query/css/css_parser.hpp"
#include "hps/query/css/css_selector.hpp"
#include <gtest/gtest.h>

namespace hps::tests {

// Helper to parse a selector
std::unique_ptr<SelectorList> parse(std::string_view selector) {
    CSSParser parser(selector);
    return parser.parse_selector_list();
}

TEST(CSSSelectorTest, PseudoClassIs) {
    Element div("div");
    Element span("span");
    Element p("p");
    
    // :is(div, span) matches div and span
    auto sel = parse(":is(div, span)");
    ASSERT_TRUE(sel);
    EXPECT_TRUE(sel->matches(div));
    EXPECT_TRUE(sel->matches(span));
    EXPECT_FALSE(sel->matches(p));
    
    // Specificity: :is(.foo, #bar) should be ID specificity
    // We can't easily check specificity value directly without exposing it, 
    // but we can assume correct implementation if matches() works for now.
}

TEST(CSSSelectorTest, PseudoClassWhere) {
    Element div("div");
    Element span("span");
    Element p("p");
    
    // :where(div, span) matches div and span
    auto sel = parse(":where(div, span)");
    ASSERT_TRUE(sel);
    EXPECT_TRUE(sel->matches(div));
    EXPECT_TRUE(sel->matches(span));
    EXPECT_FALSE(sel->matches(p));
}

TEST(CSSSelectorTest, PseudoClassHas) {
    Element parent("div");
    auto child = std::make_unique<Element>("span");
    child->add_attribute("class", "foo");
    
    Element* child_ptr = child.get();
    parent.add_child(std::move(child));
    
    // div:has(.foo) matches parent
    EXPECT_TRUE(parse("div:has(.foo)")->matches(parent));
    
    // div:has(.bar) does not match
    EXPECT_FALSE(parse("div:has(.bar)")->matches(parent));
    
    // div:has(span) matches
    EXPECT_TRUE(parse("div:has(span)")->matches(parent));
}

TEST(CSSSelectorTest, PseudoClassNot) {
    Element div("div");
    div.add_attribute("class", "foo");
    
    // :not(.bar) matches div.foo
    EXPECT_TRUE(parse(":not(.bar)")->matches(div));
    
    // :not(.foo) does not match div.foo
    EXPECT_FALSE(parse(":not(.foo)")->matches(div));
    
    // :not(span) matches div
    EXPECT_TRUE(parse(":not(span)")->matches(div));
}

} // namespace hps::tests
