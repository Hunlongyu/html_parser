#include "hps/hps.hpp"
#include "hps/core/document.hpp"
#include "hps/core/element.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace hps::tests {
namespace {

// 构造 <ul><li>1</li>...<li>10</li></ul>
std::shared_ptr<Document> make_list(int count) {
    std::string html = "<ul>";
    for (int i = 1; i <= count; ++i) {
        html += "<li>" + std::to_string(i) + "</li>";
    }
    html += "</ul>";
    return hps::parse(html);
}

std::vector<std::string> matched_positions(const std::shared_ptr<Document>& doc, std::string_view selector) {
    std::vector<std::string> positions;
    for (const auto* element : doc->query_selector_all(selector)) {
        positions.push_back(element->text_content());
    }
    return positions;
}

using Strings = std::vector<std::string>;

}  // namespace

TEST(NthChild, KeywordsOddEven) {
    const auto doc = make_list(10);
    EXPECT_EQ(matched_positions(doc, "li:nth-child(odd)"), (Strings{"1", "3", "5", "7", "9"}));
    EXPECT_EQ(matched_positions(doc, "li:nth-child(even)"), (Strings{"2", "4", "6", "8", "10"}));
}

TEST(NthChild, AnPlusBForms) {
    const auto doc = make_list(10);
    EXPECT_EQ(matched_positions(doc, "li:nth-child(2n+1)"), (Strings{"1", "3", "5", "7", "9"}));
    EXPECT_EQ(matched_positions(doc, "li:nth-child(2n)"), (Strings{"2", "4", "6", "8", "10"}));
    EXPECT_EQ(matched_positions(doc, "li:nth-child(3n)"), (Strings{"3", "6", "9"}));
    EXPECT_EQ(matched_positions(doc, "li:nth-child(3n+1)"), (Strings{"1", "4", "7", "10"}));
}

TEST(NthChild, PlainIntegerSelectsSinglePosition) {
    const auto doc = make_list(10);
    EXPECT_EQ(matched_positions(doc, "li:nth-child(3)"), (Strings{"3"}));
    EXPECT_EQ(matched_positions(doc, "li:nth-child(0n+5)"), (Strings{"5"}));
}

TEST(NthChild, NegativeCoefficientSelectsLeadingRange) {
    const auto doc = make_list(10);
    EXPECT_EQ(matched_positions(doc, "li:nth-child(-n+3)"), (Strings{"1", "2", "3"}));
}

TEST(NthChild, OffsetSelectsTrailingRange) {
    const auto doc = make_list(10);
    EXPECT_EQ(matched_positions(doc, "li:nth-child(n+7)"), (Strings{"7", "8", "9", "10"}));
}

TEST(NthChild, BarePlusNAndNSelectAll) {
    const auto doc = make_list(4);
    EXPECT_EQ(matched_positions(doc, "li:nth-child(n)"), (Strings{"1", "2", "3", "4"}));
    EXPECT_EQ(matched_positions(doc, "li:nth-child(+n)"), (Strings{"1", "2", "3", "4"}));
}

TEST(NthChild, WhitespaceInExpressionIsTolerated) {
    const auto doc = make_list(10);
    EXPECT_EQ(matched_positions(doc, "li:nth-child( 2n + 1 )"), (Strings{"1", "3", "5", "7", "9"}));
}

TEST(NthChild, InvalidExpressionMatchesNothing) {
    const auto doc = make_list(10);
    EXPECT_TRUE(matched_positions(doc, "li:nth-child(abc)").empty());
    EXPECT_TRUE(matched_positions(doc, "li:nth-child(2x+1)").empty());
}

TEST(NthLastChild, ReverseIndexing) {
    const auto doc = make_list(10);
    EXPECT_EQ(matched_positions(doc, "li:nth-last-child(1)"), (Strings{"10"}));
    EXPECT_EQ(matched_positions(doc, "li:nth-last-child(2)"), (Strings{"9"}));
    // 倒数奇数位 → 正向 10,8,6,4,2
    EXPECT_EQ(matched_positions(doc, "li:nth-last-child(odd)"), (Strings{"2", "4", "6", "8", "10"}));
}

}  // namespace hps::tests
