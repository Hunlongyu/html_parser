#include "hps/hps.hpp"
#include "hps/core/document.hpp"
#include "hps/core/element.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace hps::tests {
namespace {

const Element* first(const std::shared_ptr<Document>& doc, std::string_view selector) {
    return doc->query_selector(selector);
}

}  // namespace

// ==================== outer_html / inner_html 基础 ====================

TEST(Serialization, OuterAndInnerHtmlBasic) {
    const auto doc = hps::parse(R"(<div id='a' class='x'><p>Hi</p></div>)");
    const auto* div = first(doc, "div");
    ASSERT_NE(div, nullptr);

    EXPECT_EQ(div->outer_html(), R"(<div id="a" class="x"><p>Hi</p></div>)");
    EXPECT_EQ(div->inner_html(), "<p>Hi</p>");
}

TEST(Serialization, AttributeOrderPreservedAndQuotedWithDoubleQuotes) {
    const auto doc = hps::parse(R"(<a href='/p' data-id='7'>x</a>)");
    const auto* a = first(doc, "a");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->outer_html(), R"(<a href="/p" data-id="7">x</a>)");
}

TEST(Serialization, BooleanAttributeHasNoValue) {
    const auto doc = hps::parse("<input disabled>");
    const auto* input = first(doc, "input");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->outer_html(), "<input disabled>");
}

TEST(Serialization, VoidElementsHaveNoClosingTag) {
    const auto doc = hps::parse(R"(<div><br><img src='a.png'></div>)");
    const auto* div = first(doc, "div");
    ASSERT_NE(div, nullptr);
    EXPECT_EQ(div->inner_html(), R"(<br><img src="a.png">)");
}

TEST(Serialization, CommentsAreSerialized) {
    const auto doc = hps::parse("<div><!-- hi --></div>");
    const auto* div = first(doc, "div");
    ASSERT_NE(div, nullptr);
    EXPECT_EQ(div->inner_html(), "<!-- hi -->");
}

// ==================== DOCTYPE 节点 ====================

TEST(Serialization, DoctypeNodeIsExposedAndTyped) {
    const auto doc = hps::parse("<!DOCTYPE html><p>x</p>");
    const auto* first_node = doc->first_child();
    ASSERT_NE(first_node, nullptr);
    ASSERT_TRUE(first_node->is_doctype());
    const auto* doctype = first_node->as_doctype();
    ASSERT_NE(doctype, nullptr);
    EXPECT_EQ(doctype->name(), "html");
    EXPECT_FALSE(doctype->has_identifiers());
    EXPECT_TRUE(doctype->public_id().empty());
    EXPECT_TRUE(doctype->system_id().empty());
}

TEST(Serialization, DoctypeParsesPublicAndSystemIdentifiers) {
    const auto doc = hps::parse(
        R"(<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://x/strict.dtd">)");
    const auto* doctype = doc->first_child() ? doc->first_child()->as_doctype() : nullptr;
    ASSERT_NE(doctype, nullptr);
    EXPECT_EQ(doctype->name(), "html");
    EXPECT_TRUE(doctype->has_identifiers());
    EXPECT_EQ(doctype->public_id(), "-//W3C//DTD HTML 4.01//EN");
    EXPECT_EQ(doctype->system_id(), "http://x/strict.dtd");
}

TEST(Serialization, DoctypeKeywordIsCaseInsensitive) {
    const auto doc = hps::parse("<!dOcTyPe HtMl>");
    const auto* doctype = doc->first_child() ? doc->first_child()->as_doctype() : nullptr;
    ASSERT_NE(doctype, nullptr);
    EXPECT_EQ(doctype->name(), "html");  // 名称按 HTML5 规范小写化
}

TEST(Serialization, OuterHtmlIncludesDoctype) {
    const auto doc = hps::parse("<!DOCTYPE html><html><head></head><body>hi</body></html>");
    EXPECT_EQ(doc->outer_html(), "<!DOCTYPE html><html><head></head><body>hi</body></html>");
}

TEST(Serialization, NestedStructureRoundTrips) {
    const auto doc = hps::parse(
        R"(<ul class="m"><li>1</li><li><b>2</b></li></ul>)");
    const auto* ul = first(doc, "ul");
    ASSERT_NE(ul, nullptr);
    EXPECT_EQ(ul->outer_html(), R"(<ul class="m"><li>1</li><li><b>2</b></li></ul>)");
}

// ==================== 转义行为（实体感知） ====================

TEST(Serialization, ExistingEntitiesAreNotDoubleEscaped) {
    // 默认（未解码实体）模式：源中的 &amp;/&lt; 应原样保留，不被二次转义。
    const auto doc = hps::parse("<p>a &amp; b &lt; c</p>");
    const auto* p = first(doc, "p");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->inner_html(), "a &amp; b &lt; c");
}

TEST(Serialization, BareAmpersandIsEscaped) {
    const auto doc = hps::parse("<p>Tom & Jerry</p>");
    const auto* p = first(doc, "p");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->inner_html(), "Tom &amp; Jerry");
}

TEST(Serialization, AttributeValueEscapesQuotesAndKeepsEntities) {
    const auto doc = hps::parse(R"(<a title='say "hi"' href="?a=1&amp;b=2">x</a>)");
    const auto* a = first(doc, "a");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->outer_html(), R"(<a title="say &quot;hi&quot;" href="?a=1&amp;b=2">x</a>)");
}

TEST(Serialization, RawTextElementContentIsNotEscaped) {
    const auto doc = hps::parse("<script>if (a < b && c) x();</script>");
    const auto* script = first(doc, "script");
    ASSERT_NE(script, nullptr);
    EXPECT_EQ(script->inner_html(), "if (a < b && c) x();");
    EXPECT_EQ(script->outer_html(), "<script>if (a < b && c) x();</script>");
}

TEST(Serialization, DecodedEntitiesAreReEscapedForValidOutput) {
    // 开启实体解码后，文本中会出现裸 '<'/'&'，序列化时应重新转义，保证可被安全回解析。
    Options opts;
    opts.decode_entities = true;
    const auto doc = hps::parse("<p>5 &lt; 6 &amp; 7</p>", opts);
    const auto* p = first(doc, "p");
    ASSERT_NE(p, nullptr);
    ASSERT_EQ(p->text_content(), "5 < 6 & 7");  // 已解码
    EXPECT_EQ(p->inner_html(), "5 &lt; 6 &amp; 7");
}

// ==================== Document / ElementQuery ====================

TEST(Serialization, DocumentOuterHtmlRoundTrips) {
    const auto doc = hps::parse("<html><head></head><body><p>x</p></body></html>");
    const std::string html = doc->outer_html();
    EXPECT_NE(html.find("<p>x</p>"), std::string::npos);

    // 再次解析序列化结果应得到等价文档。
    const auto reparsed = hps::parse(html);
    const auto* p = reparsed->query_selector("p");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->text_content(), "x");
}

TEST(Serialization, ElementQueryHtmlReturnsInnerOfFirst) {
    const auto doc = hps::parse("<ul><li><b>a</b></li><li>b</li></ul>");
    EXPECT_EQ(doc->css("li").html(), "<b>a</b>");
    EXPECT_TRUE(doc->css("nonexistent").html().empty());
}

TEST(Serialization, ElementQueryExtractOuterHtml) {
    const auto doc = hps::parse("<ul><li>a</li><li>b</li></ul>");
    const auto all = doc->css("li").extract_outer_html();
    ASSERT_EQ(all.size(), 2u);
    EXPECT_EQ(all[0], "<li>a</li>");
    EXPECT_EQ(all[1], "<li>b</li>");
}

}  // namespace hps::tests
