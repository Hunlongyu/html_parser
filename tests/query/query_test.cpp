#include "hps/hps.hpp"
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

TEST_F(QueryTest, DocumentScopedCSSQueryCanMatchRootElement) {
    auto results = Query::css(*doc, "html");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0]->tag_name(), "html");
}

TEST_F(QueryTest, CSSQueryOnElement) {
    auto root = doc->first_child()->as_element(); // html
    ASSERT_NE(root, nullptr);
    
    auto results = Query::css(*root, "div");
    EXPECT_EQ(results.size(), 1u);
}

TEST_F(QueryTest, ElementScopedCSSQueryDoesNotMatchContextElement) {
    auto root = doc->first_child()->as_element(); // html
    ASSERT_NE(root, nullptr);

    auto results = Query::css(*root, "html");
    EXPECT_TRUE(results.empty());
}

TEST_F(QueryTest, DocumentScopedQueriesTraverseAllTopLevelElementsInFragments) {
    Document fragment("");

    auto first = std::make_unique<Element>("div");
    first->add_attribute("id", "first");

    auto second = std::make_unique<Element>("section");
    second->add_attribute("class", "wrapper");

    auto nested = std::make_unique<Element>("div");
    nested->add_attribute("class", "target");
    second->add_child(std::move(nested));

    fragment.add_child(std::move(first));
    fragment.add_child(std::move(second));

    const auto sections = Query::css(fragment, "section");
    ASSERT_EQ(sections.size(), 1u);
    EXPECT_EQ(sections[0]->tag_name(), "section");

    const auto divs = Query::css(fragment, "div");
    ASSERT_EQ(divs.size(), 2u);
    EXPECT_EQ(divs[0]->id(), "first");
    EXPECT_TRUE(divs[1]->has_class("target"));

    const auto* first_target = Query::css_first(fragment, ".target");
    ASSERT_NE(first_target, nullptr);
    EXPECT_EQ(first_target->tag_name(), "div");
}

TEST_F(QueryTest, PreserveCaseDocumentsStillUseCaseInsensitiveHtmlTagMatching) {
    Options options;
    options.preserve_case = true;

    const auto doc = parse(R"(<DIV><INPUT TYPE="CHECKBOX" CHECKED><SPAN>ok</SPAN></DIV>)", options);
    ASSERT_NE(doc, nullptr);

    const auto divs = Query::css(*doc, "div");
    ASSERT_EQ(divs.size(), 1u);
    EXPECT_EQ(divs[0]->tag_name(), "DIV");

    const auto checked = Query::css(*doc, "input:checked");
    ASSERT_EQ(checked.size(), 1u);
    EXPECT_EQ(checked[0]->tag_name(), "INPUT");
}

TEST_F(QueryTest, QuerySupportsRelativeHasSelectors) {
    const auto doc = parse(R"(
        <div class="card">
          <span class="lead"></span>
          <span class="target"></span>
        </div>
        <div class="card">
          <span class="lead"></span>
        </div>
    )");
    ASSERT_NE(doc, nullptr);

    const auto direct_child_match = Query::css(*doc, "div:has(> span.target)");
    ASSERT_EQ(direct_child_match.size(), 1u);
    EXPECT_TRUE(direct_child_match[0]->has_class("card"));

    const auto adjacent_match = Query::css(*doc, "span:has(+ span.target)");
    ASSERT_EQ(adjacent_match.size(), 1u);
    EXPECT_TRUE(adjacent_match[0]->has_class("lead"));
}

} // namespace hps::tests
