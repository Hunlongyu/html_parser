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

TEST(CSSSelectorTest, BasicTypeSelector) {
    Element div("div");
    Element span("span");
    
    auto sel = parse("div");
    ASSERT_TRUE(sel);
    EXPECT_TRUE(sel->matches(div));
    EXPECT_FALSE(sel->matches(span));
    
    auto sel_star = parse("*");
    ASSERT_TRUE(sel_star);
    EXPECT_TRUE(sel_star->matches(div));
    EXPECT_TRUE(sel_star->matches(span));
}

TEST(CSSSelectorTest, ClassSelector) {
    Element el("div");
    el.add_attribute("class", "foo bar");
    
    auto sel_foo = parse(".foo");
    ASSERT_TRUE(sel_foo);
    EXPECT_TRUE(sel_foo->matches(el));
    
    auto sel_bar = parse(".bar");
    ASSERT_TRUE(sel_bar);
    EXPECT_TRUE(sel_bar->matches(el));
    
    auto sel_baz = parse(".baz");
    ASSERT_TRUE(sel_baz);
    EXPECT_FALSE(sel_baz->matches(el));
}

TEST(CSSSelectorTest, IdSelector) {
    Element el("div");
    el.add_attribute("id", "my-id");
    
    auto sel = parse("#my-id");
    ASSERT_TRUE(sel);
    EXPECT_TRUE(sel->matches(el));
    
    auto sel_wrong = parse("#other-id");
    ASSERT_TRUE(sel_wrong);
    EXPECT_FALSE(sel_wrong->matches(el));
}

TEST(CSSSelectorTest, AttributeSelector) {
    Element el("input");
    el.add_attribute("type", "text");
    el.add_attribute("data-val", "hello world");
    
    // [attr]
    EXPECT_TRUE(parse("[type]")->matches(el));
    EXPECT_FALSE(parse("[disabled]")->matches(el));
    
    // [attr=val]
    EXPECT_TRUE(parse("[type='text']")->matches(el));
    EXPECT_FALSE(parse("[type='password']")->matches(el));
    
    // [attr~=val] (word match)
    EXPECT_TRUE(parse("[data-val~='hello']")->matches(el));
    EXPECT_TRUE(parse("[data-val~='world']")->matches(el));
    EXPECT_FALSE(parse("[data-val~='hell']")->matches(el));
    
    // [attr^=val] (starts with)
    EXPECT_TRUE(parse("[data-val^='hell']")->matches(el));
    
    // [attr$=val] (ends with)
    EXPECT_TRUE(parse("[data-val$='world']")->matches(el));
    
    // [attr*=val] (contains)
    EXPECT_TRUE(parse("[data-val*='lo wo']")->matches(el));
}

TEST(CSSSelectorTest, CombinatorSelector) {
    // Structure: div#parent > span#child + span#sibling
    Element parent("div");
    parent.add_attribute("id", "parent");
    
    auto child = std::make_unique<Element>("span");
    child->add_attribute("id", "child");
    
    auto sibling = std::make_unique<Element>("span");
    sibling->add_attribute("id", "sibling");
    
    Element* child_ptr = child.get();
    Element* sibling_ptr = sibling.get();
    
    parent.add_child(std::move(child));
    parent.add_child(std::move(sibling));
    
    // Child combinator >
    EXPECT_TRUE(parse("div > span")->matches(*child_ptr));
    
    // Descendant combinator (space)
    EXPECT_TRUE(parse("div span")->matches(*child_ptr));
    
    // Adjacent sibling +
    EXPECT_TRUE(parse("#child + span")->matches(*sibling_ptr));
    EXPECT_FALSE(parse("#child + div")->matches(*sibling_ptr));
    
    // General sibling ~
    EXPECT_TRUE(parse("#child ~ span")->matches(*sibling_ptr));
}

TEST(CSSSelectorTest, CompoundSelector) {
    Element el("div");
    el.add_attribute("class", "foo");
    el.add_attribute("id", "bar");
    
    EXPECT_TRUE(parse("div.foo#bar")->matches(el));
    EXPECT_FALSE(parse("div.foo#baz")->matches(el));
    EXPECT_FALSE(parse("span.foo#bar")->matches(el));
}

TEST(CSSSelectorTest, PseudoClassFirstChild) {
    Element parent("div");
    auto child1 = std::make_unique<Element>("p");
    auto child2 = std::make_unique<Element>("p");
    
    Element* p1 = child1.get();
    Element* p2 = child2.get();
    
    parent.add_child(std::move(child1));
    parent.add_child(std::move(child2));
    
    EXPECT_TRUE(parse("p:first-child")->matches(*p1));
    EXPECT_FALSE(parse("p:first-child")->matches(*p2));
    
    EXPECT_FALSE(parse("p:last-child")->matches(*p1));
    EXPECT_TRUE(parse("p:last-child")->matches(*p2));
}

TEST(CSSSelectorTest, GroupingSelector) {
    Element div("div");
    Element span("span");
    Element p("p");
    
    auto sel = parse("div, span");
    EXPECT_TRUE(sel->matches(div));
    EXPECT_TRUE(sel->matches(span));
    EXPECT_FALSE(sel->matches(p));
}

} // namespace hps::tests
