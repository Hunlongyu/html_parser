#include "hps/query/element_query.hpp"
#include "hps/core/element.hpp"
#include <gtest/gtest.h>
#include <memory>

namespace hps::tests {

class ElementQueryTest : public ::testing::Test {
protected:
    void SetUp() override {
        e1 = std::make_unique<Element>("div");
        e2 = std::make_unique<Element>("p");
        e3 = std::make_unique<Element>("span");
        
        raw_elements = {e1.get(), e2.get(), e3.get()};
    }
    
    std::unique_ptr<Element> e1, e2, e3;
    std::vector<const Element*> raw_elements;
};

TEST_F(ElementQueryTest, Construction) {
    ElementQuery q(raw_elements);
    EXPECT_EQ(q.size(), 3u);
    EXPECT_FALSE(q.empty());
    EXPECT_EQ(q.first_element(), e1.get());
    EXPECT_EQ(q.last_element(), e3.get());
}

TEST_F(ElementQueryTest, Filtering) {
    ElementQuery q(raw_elements);
    
    auto divs = q.has_tag("div");
    EXPECT_EQ(divs.size(), 1u);
    EXPECT_EQ(divs[0], e1.get());
    
    auto spans = q.has_tag("span");
    EXPECT_EQ(spans.size(), 1u);
    EXPECT_EQ(spans[0], e3.get());
}

TEST_F(ElementQueryTest, Slicing) {
    ElementQuery q(raw_elements);
    
    auto first2 = q.first(2);
    EXPECT_EQ(first2.size(), 2u);
    EXPECT_EQ(first2[0], e1.get());
    EXPECT_EQ(first2[1], e2.get());
    
    auto skipped = q.skip(1);
    EXPECT_EQ(skipped.size(), 2u);
    EXPECT_EQ(skipped[0], e2.get());
}

TEST_F(ElementQueryTest, Iteration) {
    ElementQuery q(raw_elements);
    int count = 0;
    
    for (auto it = q.begin(); it != q.end(); ++it) {
        count++;
    }
    EXPECT_EQ(count, 3);
    
    count = 0;
    q.each([&](const Element&) {
        count++;
    });
    EXPECT_EQ(count, 3);
}

} // namespace hps::tests
