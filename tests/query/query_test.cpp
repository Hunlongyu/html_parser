#include "hps/query/query.hpp"
#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include <gtest/gtest.h>

namespace hps::tests {

class QueryTest : public ::testing::Test {
protected:
    void SetUp() override {
        doc = std::make_shared<Document>("");
        auto root = std::make_unique<Element>("html");
        auto body = std::make_unique<Element>("body");
        auto div = std::make_unique<Element>("div");
        div->add_attribute("class", "container");
        
        body->add_child(std::move(div));
        root->add_child(std::move(body));
        doc->add_child(std::move(root));
    }

    std::shared_ptr<Document> doc;
};

TEST_F(QueryTest, CSSQueryOnDocument) {
    auto results = Query::css(*doc, ".container");
    EXPECT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0]->tag_name(), "div");
}

TEST_F(QueryTest, CSSQueryOnElement) {
    auto root = doc->first_child()->as_element(); // html
    ASSERT_NE(root, nullptr);
    
    auto results = Query::css(*root, "div");
    EXPECT_EQ(results.size(), 1u);
}

} // namespace hps::tests
