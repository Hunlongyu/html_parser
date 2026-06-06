#include "hps/core/document.hpp"
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
    Document doc("");
    Element parent("div");

    // Structure:
    // div
    //   p (1)
    //   span (1)
    //   p (2)
    //   span (2)
    //   p (3)

    Element* p1 = doc.create_element("p");
    Element* s1 = doc.create_element("span");
    Element* p2 = doc.create_element("p");
    Element* s2 = doc.create_element("span");
    Element* p3 = doc.create_element("p");

    Element* p1_ptr = p1;
    Element* s1_ptr = s1;
    Element* p2_ptr = p2;
    Element* s2_ptr = s2;
    Element* p3_ptr = p3;

    parent.add_child(p1);
    parent.add_child(s1);
    parent.add_child(p2);
    parent.add_child(s2);
    parent.add_child(p3);

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
    Document doc("");
    Element parent("div");

    // Structure:
    // div
    //   p (1) - 3rd from last
    //   span (1) - 2nd from last
    //   p (2) - 2nd from last
    //   span (2) - 1st from last
    //   p (3) - 1st from last

    Element* p1 = doc.create_element("p");
    Element* s1 = doc.create_element("span");
    Element* p2 = doc.create_element("p");
    Element* s2 = doc.create_element("span");
    Element* p3 = doc.create_element("p");

    Element* p1_ptr = p1;
    Element* s1_ptr = s1;
    Element* p2_ptr = p2;
    Element* s2_ptr = s2;
    Element* p3_ptr = p3;

    parent.add_child(p1);
    parent.add_child(s1);
    parent.add_child(p2);
    parent.add_child(s2);
    parent.add_child(p3);

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
