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

TEST(CSSSelectorTest, NthOfType) {
    Element parent("div");
    
    // Structure:
    // div
    //   p (1)
    //   span (1)
    //   p (2)
    //   span (2)
    //   p (3)
    
    auto p1 = std::make_unique<Element>("p");
    auto s1 = std::make_unique<Element>("span");
    auto p2 = std::make_unique<Element>("p");
    auto s2 = std::make_unique<Element>("span");
    auto p3 = std::make_unique<Element>("p");
    
    Element* p1_ptr = p1.get();
    Element* s1_ptr = s1.get();
    Element* p2_ptr = p2.get();
    Element* s2_ptr = s2.get();
    Element* p3_ptr = p3.get();
    
    parent.add_child(std::move(p1));
    parent.add_child(std::move(s1));
    parent.add_child(std::move(p2));
    parent.add_child(std::move(s2));
    parent.add_child(std::move(p3));
    
    // :nth-of-type(1)
    EXPECT_TRUE(parse("p:nth-of-type(1)")->matches(*p1_ptr));
    EXPECT_FALSE(parse("p:nth-of-type(1)")->matches(*p2_ptr));
    EXPECT_TRUE(parse("span:nth-of-type(1)")->matches(*s1_ptr));
    
    // :nth-of-type(2)
    EXPECT_TRUE(parse("p:nth-of-type(2)")->matches(*p2_ptr));
    EXPECT_TRUE(parse("span:nth-of-type(2)")->matches(*s2_ptr));
    
    // :nth-of-type(odd) -> 1, 3
    EXPECT_TRUE(parse("p:nth-of-type(odd)")->matches(*p1_ptr));
    EXPECT_FALSE(parse("p:nth-of-type(odd)")->matches(*p2_ptr));
    EXPECT_TRUE(parse("p:nth-of-type(odd)")->matches(*p3_ptr));
    
    // :nth-of-type(even) -> 2
    EXPECT_FALSE(parse("p:nth-of-type(even)")->matches(*p1_ptr));
    EXPECT_TRUE(parse("p:nth-of-type(even)")->matches(*p2_ptr));
    EXPECT_FALSE(parse("p:nth-of-type(even)")->matches(*p3_ptr));
    
    // :nth-of-type(2n+1) -> 1, 3
    EXPECT_TRUE(parse("p:nth-of-type(2n+1)")->matches(*p1_ptr));
    EXPECT_FALSE(parse("p:nth-of-type(2n+1)")->matches(*p2_ptr));
    EXPECT_TRUE(parse("p:nth-of-type(2n+1)")->matches(*p3_ptr));
}

TEST(CSSSelectorTest, NthLastOfType) {
    Element parent("div");
    
    // Structure:
    // div
    //   p (1) - 3rd from last
    //   span (1) - 2nd from last
    //   p (2) - 2nd from last
    //   span (2) - 1st from last
    //   p (3) - 1st from last
    
    auto p1 = std::make_unique<Element>("p");
    auto s1 = std::make_unique<Element>("span");
    auto p2 = std::make_unique<Element>("p");
    auto s2 = std::make_unique<Element>("span");
    auto p3 = std::make_unique<Element>("p");
    
    Element* p1_ptr = p1.get();
    Element* s1_ptr = s1.get();
    Element* p2_ptr = p2.get();
    Element* s2_ptr = s2.get();
    Element* p3_ptr = p3.get();
    
    parent.add_child(std::move(p1));
    parent.add_child(std::move(s1));
    parent.add_child(std::move(p2));
    parent.add_child(std::move(s2));
    parent.add_child(std::move(p3));
    
    // :nth-last-of-type(1)
    EXPECT_TRUE(parse("p:nth-last-of-type(1)")->matches(*p3_ptr));
    EXPECT_TRUE(parse("span:nth-last-of-type(1)")->matches(*s2_ptr));
    EXPECT_FALSE(parse("p:nth-last-of-type(1)")->matches(*p2_ptr));
    
    // :nth-last-of-type(2)
    EXPECT_TRUE(parse("p:nth-last-of-type(2)")->matches(*p2_ptr));
    EXPECT_TRUE(parse("span:nth-last-of-type(2)")->matches(*s1_ptr));
    
    // :nth-last-of-type(3)
    EXPECT_TRUE(parse("p:nth-last-of-type(3)")->matches(*p1_ptr));
}

} // namespace hps::tests
