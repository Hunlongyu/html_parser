#include "hps/hps.hpp"
#include "hps/core/element.hpp"
#include "hps/core/text_node.hpp"

#include <filesystem>
#include <fstream>
#include <ranges>
#include <string>

#include <gtest/gtest.h>

namespace {
bool has_error_code(const std::vector<hps::HPSError>& errors, const hps::ErrorCode code) {
    return std::ranges::any_of(errors, [code](const hps::HPSError& err) { return err.code == code; });
}

void write_binary_file(const std::filesystem::path& path, const std::string& bytes) {
    std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    ASSERT_TRUE(out.good());
}

auto utf16le_with_bom(std::u16string_view text) -> std::string {
    std::string bytes("\xFF\xFE", 2);
    bytes.reserve(bytes.size() + text.size() * 2);
    for (const char16_t code_unit : text) {
        bytes.push_back(static_cast<char>(code_unit & 0xFF));
        bytes.push_back(static_cast<char>((code_unit >> 8) & 0xFF));
    }
    return bytes;
}
}  // namespace

TEST(HTMLParser, EnforcesMaxTokens) {
    hps::Options opts;
    opts.max_tokens     = 3;
    opts.error_handling = hps::ErrorHandlingMode::Lenient;

    const std::string html = "<a></a><b></b><c></c><d></d>";
    const auto        res  = hps::parse_with_error(html, opts);

    EXPECT_TRUE(has_error_code(res.errors, hps::ErrorCode::TooManyElements));
}

TEST(HTMLParser, ParseFileUsesFileContent) {
    const auto temp_path = std::filesystem::temp_directory_path() / "hps_html_parser_test.html";
    const std::string html = "<div>Hello</div>";

    write_binary_file(temp_path, html);

    const auto res = hps::parse_file_with_error(temp_path.string(), hps::Options{});
    ASSERT_NE(res.document, nullptr);
    EXPECT_EQ(res.document->source_html(), html);

    std::error_code ec;
    std::filesystem::remove(temp_path, ec);
}

TEST(HTMLParser, ParseFileStripsUtf8Bom) {
    const auto temp_path = std::filesystem::temp_directory_path() / "hps_html_parser_test_utf8_bom.html";
    const std::string html = "<div>Hello</div>";
    write_binary_file(temp_path, std::string("\xEF\xBB\xBF", 3) + html);

    const auto res = hps::parse_file_with_error(temp_path.string(), hps::Options{});
    ASSERT_NE(res.document, nullptr);
    EXPECT_EQ(res.document->source_html(), html);

    const auto* div = res.document->querySelector("div");
    ASSERT_NE(div, nullptr);
    EXPECT_EQ(div->text_content(), "Hello");

    std::error_code ec;
    std::filesystem::remove(temp_path, ec);
}

TEST(HTMLParser, ParseFileRejectsUtf16LeBomByDefault) {
    const auto temp_path = std::filesystem::temp_directory_path() / "hps_html_parser_test_utf16le.html";
    write_binary_file(temp_path, utf16le_with_bom(u"<div>Hello</div>"));

    const auto res = hps::parse_file_with_error(temp_path.string(), hps::Options{});
    ASSERT_NE(res.document, nullptr);
    EXPECT_TRUE(has_error_code(res.errors, hps::ErrorCode::UnsupportedEncoding));
    EXPECT_TRUE(res.document->source_html().empty());

    std::error_code ec;
    std::filesystem::remove(temp_path, ec);
}

TEST(HTMLParser, ParseFileRejectsDeclaredGbkCharsetByDefault) {
    const auto temp_path = std::filesystem::temp_directory_path() / "hps_html_parser_test_gbk.html";

    std::string bytes = "<html><head><meta charset='gbk'></head><body><p>";
    bytes.append("\xD6\xD0\xCE\xC4", 4);
    bytes += "</p></body></html>";
    write_binary_file(temp_path, bytes);

    const auto res = hps::parse_file_with_error(temp_path.string(), hps::Options{});
    ASSERT_NE(res.document, nullptr);
    EXPECT_TRUE(has_error_code(res.errors, hps::ErrorCode::UnsupportedEncoding));
    EXPECT_TRUE(res.document->source_html().empty());

    std::error_code ec;
    std::filesystem::remove(temp_path, ec);
}

TEST(HTMLParser, PreserveWhitespaceOnlyTextNodesByDefault) {
    const auto document = hps::parse("<div>   </div>");
    ASSERT_NE(document, nullptr);

    const auto* div = document->querySelector("div");
    ASSERT_NE(div, nullptr);

    const auto children = div->children();
    ASSERT_EQ(children.size(), 1u);
    ASSERT_TRUE(children.front()->is_text());
    EXPECT_EQ(children.front()->as_text()->value(), "   ");
}

TEST(HTMLParser, AutoWrapsDocumentWithHtmlHeadAndBody) {
    const auto document = hps::parse("<div>Hello</div>");
    ASSERT_NE(document, nullptr);

    const auto* html = document->html();
    ASSERT_NE(html, nullptr);
    ASSERT_NE(document->querySelector("head"), nullptr);

    const auto* body = document->querySelector("body");
    ASSERT_NE(body, nullptr);

    const auto* div = body->querySelector("div");
    ASSERT_NE(div, nullptr);
    EXPECT_EQ(div->text_content(), "Hello");
}

TEST(HTMLParser, HeadContentBeforeBodyIsPlacedInHead) {
    const auto document = hps::parse("<meta charset='utf-8'><title>Example</title><p>Body</p>");
    ASSERT_NE(document, nullptr);

    const auto* head = document->querySelector("head");
    const auto* body = document->querySelector("body");
    ASSERT_NE(head, nullptr);
    ASSERT_NE(body, nullptr);

    ASSERT_NE(head->querySelector("meta"), nullptr);
    const auto* title = head->querySelector("title");
    ASSERT_NE(title, nullptr);
    EXPECT_EQ(title->text_content(), "Example");

    const auto* paragraph = body->querySelector("p");
    ASSERT_NE(paragraph, nullptr);
    EXPECT_EQ(paragraph->text_content(), "Body");
}

TEST(HTMLParser, NonWhitespaceTextInsideHeadFallsBackToBody) {
    const auto document = hps::parse("<head>orphan<title>Example</title></head><p>Body</p>");
    ASSERT_NE(document, nullptr);

    const auto* head = document->querySelector("head");
    const auto* body = document->querySelector("body");
    ASSERT_NE(head, nullptr);
    ASSERT_NE(body, nullptr);

    const auto* title = head->querySelector("title");
    EXPECT_EQ(title, nullptr);
    EXPECT_NE(body->text_content().find("orphan"), std::string::npos);
    EXPECT_NE(body->text_content().find("Example"), std::string::npos);
    EXPECT_NE(body->text_content().find("Body"), std::string::npos);
}

TEST(HTMLParser, SvgIsPreservedAsOpaqueRawContent) {
    const auto document = hps::parse("<svg viewBox='0 0 1 1'><g><title>x</title></g></svg><p>after</p>");
    ASSERT_NE(document, nullptr);

    const auto* svg = document->querySelector("svg");
    const auto* paragraph = document->querySelector("p");
    ASSERT_NE(svg, nullptr);
    ASSERT_NE(paragraph, nullptr);

    EXPECT_EQ(svg->namespace_kind(), hps::NamespaceKind::Svg);
    EXPECT_EQ(svg->get_attribute("viewbox"), "0 0 1 1");
    EXPECT_EQ(svg->text_content(), "<g><title>x</title></g>");
    EXPECT_EQ(document->querySelector("svg g"), nullptr);
    EXPECT_EQ(document->querySelector("svg title"), nullptr);
    EXPECT_EQ(paragraph->text_content(), "after");
}

TEST(HTMLParser, MathDescendantsStillCarryMathNamespace) {
    const auto document = hps::parse("<math><mi>y</mi></math>");
    ASSERT_NE(document, nullptr);

    const auto* math = document->querySelector("math");
    const auto* mi = document->querySelector("math mi");
    ASSERT_NE(math, nullptr);
    ASSERT_NE(mi, nullptr);

    EXPECT_EQ(math->namespace_kind(), hps::NamespaceKind::MathML);
    EXPECT_EQ(mi->namespace_kind(), hps::NamespaceKind::MathML);
}

TEST(HTMLParser, RawtextAndRcdataContentsStayAsText) {
    const auto document = hps::parse(R"(
        <html>
          <head>
            <style><b>x</b></style>
            <title><i>y</i></title>
          </head>
          <body>
            <textarea><span>z</span></textarea>
          </body>
        </html>
    )");
    ASSERT_NE(document, nullptr);

    const auto* style = document->querySelector("style");
    const auto* title = document->querySelector("title");
    const auto* textarea = document->querySelector("textarea");

    ASSERT_NE(style, nullptr);
    ASSERT_NE(title, nullptr);
    ASSERT_NE(textarea, nullptr);

    EXPECT_EQ(style->text_content(), "<b>x</b>");
    EXPECT_EQ(title->text_content(), "<i>y</i>");
    EXPECT_EQ(textarea->text_content(), "<span>z</span>");

    EXPECT_EQ(document->querySelector("style b"), nullptr);
    EXPECT_EQ(document->querySelector("title i"), nullptr);
    EXPECT_EQ(document->querySelector("textarea span"), nullptr);
}

TEST(HTMLParser, RcdataDecodesEntitiesDuringTokenization) {
    const auto document = hps::parse("<textarea>&lt;&amp;</textarea><title>&lt;</title>");
    ASSERT_NE(document, nullptr);

    const auto* textarea = document->querySelector("textarea");
    const auto* title = document->querySelector("title");
    ASSERT_NE(textarea, nullptr);
    ASSERT_NE(title, nullptr);

    EXPECT_EQ(textarea->text_content(), "<&");
    EXPECT_EQ(title->text_content(), "<");
}

TEST(HTMLParser, TableRowsAutoInsertTbody) {
    const auto document = hps::parse("<table><tr><td>a</td><td>b</td></tr></table>");
    ASSERT_NE(document, nullptr);

    const auto* table = document->querySelector("table");
    ASSERT_NE(table, nullptr);

    const auto* tbody = table->querySelector("tbody");
    ASSERT_NE(tbody, nullptr);

    const auto* row = tbody->querySelector("tr");
    ASSERT_NE(row, nullptr);

    const auto cells = row->querySelectorAll("td");
    ASSERT_EQ(cells.size(), 2u);
    EXPECT_EQ(cells[0]->text_content(), "a");
    EXPECT_EQ(cells[1]->text_content(), "b");
}

TEST(HTMLParser, TableCellsAutoInsertRowAndClosePreviousCell) {
    const auto document = hps::parse("<table><td>a<td>b</table>");
    ASSERT_NE(document, nullptr);

    const auto* row = document->querySelector("table tbody tr");
    ASSERT_NE(row, nullptr);

    const auto cells = row->querySelectorAll("td");
    ASSERT_EQ(cells.size(), 2u);
    EXPECT_EQ(cells[0]->text_content(), "a");
    EXPECT_EQ(cells[1]->text_content(), "b");
}

TEST(HTMLParser, SelectOptionsImplicitlyClosePreviousOption) {
    const auto document = hps::parse("<select><option>a<option>b</select>");
    ASSERT_NE(document, nullptr);

    const auto* select = document->querySelector("select");
    ASSERT_NE(select, nullptr);

    const auto children = select->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "option");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "option");
    EXPECT_EQ(children[0]->as_element()->text_content(), "a");
    EXPECT_EQ(children[1]->as_element()->text_content(), "b");
}

TEST(HTMLParser, SelectOptgroupClosesOpenOptionAndPreviousOptgroup) {
    const auto document =
        hps::parse("<select><optgroup label=\"one\"><option>a<optgroup label=\"two\"><option>b</select>");
    ASSERT_NE(document, nullptr);

    const auto* select = document->querySelector("select");
    ASSERT_NE(select, nullptr);

    const auto children = select->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "optgroup");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "optgroup");
    EXPECT_EQ(children[0]->as_element()->get_attribute("label"), "one");
    EXPECT_EQ(children[1]->as_element()->get_attribute("label"), "two");

    const auto first_group_children = children[0]->children();
    const auto second_group_children = children[1]->children();
    ASSERT_EQ(first_group_children.size(), 1u);
    ASSERT_EQ(second_group_children.size(), 1u);
    ASSERT_TRUE(first_group_children[0]->is_element());
    ASSERT_TRUE(second_group_children[0]->is_element());
    EXPECT_EQ(first_group_children[0]->as_element()->tag_name(), "option");
    EXPECT_EQ(second_group_children[0]->as_element()->tag_name(), "option");
    EXPECT_EQ(first_group_children[0]->as_element()->text_content(), "a");
    EXPECT_EQ(second_group_children[0]->as_element()->text_content(), "b");
}

TEST(HTMLParser, ButtonStartTagImplicitlyClosesOpenButton) {
    const auto document = hps::parse("<div><button>one<button>two</div>");
    ASSERT_NE(document, nullptr);

    const auto* div = document->querySelector("div");
    ASSERT_NE(div, nullptr);

    const auto children = div->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "button");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "button");
    EXPECT_EQ(children[0]->as_element()->text_content(), "one");
    EXPECT_EQ(children[1]->as_element()->text_content(), "two");
}

TEST(HTMLParser, DuplicateAnchorStartTagClosesPreviousAnchor) {
    const auto document = hps::parse("<div><a href=\"one\">one<a href=\"two\">two</a></div>");
    ASSERT_NE(document, nullptr);

    const auto* div = document->querySelector("div");
    ASSERT_NE(div, nullptr);

    const auto children = div->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "a");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "a");
    EXPECT_EQ(children[0]->as_element()->get_attribute("href"), "one");
    EXPECT_EQ(children[1]->as_element()->get_attribute("href"), "two");
    EXPECT_EQ(children[0]->as_element()->text_content(), "one");
    EXPECT_EQ(children[1]->as_element()->text_content(), "two");
}

TEST(HTMLParser, NestedFormStartTagIsIgnored) {
    const auto document =
        hps::parse("<div><form id=\"outer\"><input name=\"a\"><form id=\"inner\"><input name=\"b\"></form></form></div>");
    ASSERT_NE(document, nullptr);

    const auto forms = document->querySelectorAll("form");
    ASSERT_EQ(forms.size(), 1u);
    EXPECT_EQ(forms[0]->id(), "outer");
    EXPECT_EQ(forms[0]->querySelectorAll("input").size(), 2u);
}

TEST(HTMLParser, TableCaptionClosesBeforeBodyRows) {
    const auto document = hps::parse("<table><caption>cap<tr><td>x</table>");
    ASSERT_NE(document, nullptr);

    const auto* table = document->querySelector("table");
    ASSERT_NE(table, nullptr);

    const auto children = table->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "caption");
    EXPECT_EQ(children[0]->as_element()->text_content(), "cap");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "tbody");

    const auto* cell = document->querySelector("table tbody tr td");
    ASSERT_NE(cell, nullptr);
    EXPECT_EQ(cell->text_content(), "x");
}

TEST(HTMLParser, NestedTableInsideCellRemainsInsideCell) {
    const auto document = hps::parse("<table><tr><td>a<table><tr><td>b</td></tr></table>c</td></tr></table>");
    ASSERT_NE(document, nullptr);

    const auto* outer_row = document->querySelector("table > tbody > tr");
    ASSERT_NE(outer_row, nullptr);

    const auto row_children = outer_row->children();
    ASSERT_EQ(row_children.size(), 1u);
    ASSERT_TRUE(row_children[0]->is_element());
    EXPECT_EQ(row_children[0]->as_element()->tag_name(), "td");

    const auto* outer_cell = row_children[0]->as_element();
    const auto children = outer_cell->children();
    ASSERT_EQ(children.size(), 3u);
    ASSERT_TRUE(children[0]->is_text());
    ASSERT_TRUE(children[1]->is_element());
    ASSERT_TRUE(children[2]->is_text());
    EXPECT_EQ(children[0]->as_text()->value(), "a");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "table");
    EXPECT_EQ(children[2]->as_text()->value(), "c");

    const auto* nested_cell = outer_cell->querySelector("table tbody tr td");
    ASSERT_NE(nested_cell, nullptr);
    EXPECT_EQ(nested_cell->text_content(), "b");
}

TEST(HTMLParser, TableColImpliesColgroup) {
    const auto document = hps::parse("<table><col><col></table>");
    ASSERT_NE(document, nullptr);

    const auto* table = document->querySelector("table");
    ASSERT_NE(table, nullptr);

    const auto children = table->children();
    ASSERT_EQ(children.size(), 1u);
    ASSERT_TRUE(children[0]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "colgroup");

    const auto colgroup_children = children[0]->children();
    ASSERT_EQ(colgroup_children.size(), 2u);
    ASSERT_TRUE(colgroup_children[0]->is_element());
    ASSERT_TRUE(colgroup_children[1]->is_element());
    EXPECT_EQ(colgroup_children[0]->as_element()->tag_name(), "col");
    EXPECT_EQ(colgroup_children[1]->as_element()->tag_name(), "col");
}

TEST(HTMLParser, ColgroupFallbackReprocessesNonColTokensInTableMode) {
    const auto document = hps::parse("<section><table><colgroup><div>x</div><tr><td>y</table></section>");
    ASSERT_NE(document, nullptr);

    const auto* section = document->querySelector("section");
    ASSERT_NE(section, nullptr);

    const auto children = section->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "div");
    EXPECT_EQ(children[0]->as_element()->text_content(), "x");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "table");

    const auto table_children = children[1]->children();
    ASSERT_EQ(table_children.size(), 2u);
    ASSERT_TRUE(table_children[0]->is_element());
    ASSERT_TRUE(table_children[1]->is_element());
    EXPECT_EQ(table_children[0]->as_element()->tag_name(), "colgroup");
    EXPECT_EQ(table_children[1]->as_element()->tag_name(), "tbody");
}

TEST(HTMLParser, ParseFragmentDoesNotAutoWrapDocumentShell) {
    const auto document = hps::parse_fragment("alpha<span>beta</span>", "div");
    ASSERT_NE(document, nullptr);

    EXPECT_EQ(document->html(), nullptr);
    EXPECT_EQ(document->text_content(), "alphabeta");

    const auto* span = document->querySelector("span");
    ASSERT_NE(span, nullptr);
    EXPECT_EQ(span->text_content(), "beta");
}

TEST(HTMLParser, ParseFragmentSvgContextTreatsContentAsOpaqueText) {
    const auto document = hps::parse_fragment("<g><title>x</title></g>", "svg");
    ASSERT_NE(document, nullptr);

    const auto children = document->children();
    ASSERT_EQ(children.size(), 1u);
    ASSERT_TRUE(children[0]->is_text());
    EXPECT_EQ(children[0]->as_text()->value(), "<g><title>x</title></g>");
    EXPECT_EQ(document->querySelector("g"), nullptr);
    EXPECT_EQ(document->text_content(), "<g><title>x</title></g>");
}

TEST(HTMLParser, ParseFragmentMathContextInheritsMathNamespace) {
    const auto document = hps::parse_fragment("<mi>x</mi>", "math");
    ASSERT_NE(document, nullptr);

    const auto* mi = document->querySelector("mi");
    ASSERT_NE(mi, nullptr);
    EXPECT_EQ(mi->namespace_kind(), hps::NamespaceKind::MathML);
}

TEST(HTMLParser, ParseFragmentUsesRcdataContextState) {
    const auto document = hps::parse_fragment("&lt;<b>x</b>", "textarea");
    ASSERT_NE(document, nullptr);

    EXPECT_EQ(document->text_content(), "<<b>x</b>");
    EXPECT_EQ(document->querySelector("b"), nullptr);
}

TEST(HTMLParser, ParseFragmentSelectContextClosesOptionsAndOptgroups) {
    const auto document =
        hps::parse_fragment("<optgroup label=\"one\"><option>a<optgroup label=\"two\"><option>b", "select");
    ASSERT_NE(document, nullptr);

    const auto children = document->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "optgroup");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "optgroup");
    EXPECT_EQ(children[0]->as_element()->get_attribute("label"), "one");
    EXPECT_EQ(children[1]->as_element()->get_attribute("label"), "two");
}

TEST(HTMLParser, ParseFragmentImplicitlyClosesOpenButton) {
    const auto document = hps::parse_fragment("<button>one<button>two", "div");
    ASSERT_NE(document, nullptr);

    const auto children = document->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "button");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "button");
    EXPECT_EQ(children[0]->as_element()->text_content(), "one");
    EXPECT_EQ(children[1]->as_element()->text_content(), "two");
}

TEST(HTMLParser, ParseFragmentDuplicateAnchorStartTagClosesPreviousAnchor) {
    const auto document = hps::parse_fragment("<a href=\"one\">one<a href=\"two\">two</a>", "div");
    ASSERT_NE(document, nullptr);

    const auto children = document->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "a");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "a");
    EXPECT_EQ(children[0]->as_element()->get_attribute("href"), "one");
    EXPECT_EQ(children[1]->as_element()->get_attribute("href"), "two");
    EXPECT_EQ(children[0]->as_element()->text_content(), "one");
    EXPECT_EQ(children[1]->as_element()->text_content(), "two");
}

TEST(HTMLParser, ParseFragmentSupportsTableContext) {
    const auto document = hps::parse_fragment("<tr><td>a<td>b", "table");
    ASSERT_NE(document, nullptr);

    const auto* tbody = document->querySelector("tbody");
    ASSERT_NE(tbody, nullptr);

    const auto* row = tbody->querySelector("tr");
    ASSERT_NE(row, nullptr);

    const auto cells = row->querySelectorAll("td");
    ASSERT_EQ(cells.size(), 2u);
    EXPECT_EQ(cells[0]->text_content(), "a");
    EXPECT_EQ(cells[1]->text_content(), "b");
}

TEST(HTMLParser, ParseFragmentTableContextSupportsCaptionAndColgroup) {
    const auto document = hps::parse_fragment("<caption>cap<col>", "table");
    ASSERT_NE(document, nullptr);

    const auto children = document->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "caption");
    EXPECT_EQ(children[0]->as_element()->text_content(), "cap");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "colgroup");

    const auto colgroup_children = children[1]->children();
    ASSERT_EQ(colgroup_children.size(), 1u);
    ASSERT_TRUE(colgroup_children[0]->is_element());
    EXPECT_EQ(colgroup_children[0]->as_element()->tag_name(), "col");
}

TEST(HTMLParser, ParseFragmentTableContextReprocessesColgroupFallback) {
    const auto document = hps::parse_fragment("<colgroup><div>x</div><tr><td>y", "table");
    ASSERT_NE(document, nullptr);

    const auto children = document->children();
    ASSERT_EQ(children.size(), 3u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    ASSERT_TRUE(children[2]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "div");
    EXPECT_EQ(children[0]->as_element()->text_content(), "x");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "colgroup");
    EXPECT_EQ(children[2]->as_element()->tag_name(), "tbody");
}

TEST(HTMLParser, TableTextBeforeRowIsFosterParentedBeforeTable) {
    const auto document = hps::parse("<div><table>alpha<tr><td>beta</td></tr></table></div>");
    ASSERT_NE(document, nullptr);

    const auto* div = document->querySelector("div");
    ASSERT_NE(div, nullptr);

    const auto children = div->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_text());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_text()->value(), "alpha");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "table");

    const auto* cell = document->querySelector("table tbody tr td");
    ASSERT_NE(cell, nullptr);
    EXPECT_EQ(cell->text_content(), "beta");
}

TEST(HTMLParser, ParseFragmentTableContextFosterParentsTextInsideFragment) {
    const auto document = hps::parse_fragment("alpha<tr><td>beta", "table");
    ASSERT_NE(document, nullptr);

    const auto children = document->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_text());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_text()->value(), "alpha");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "tbody");
}

TEST(HTMLParser, TableElementBeforeRowIsFosterParentedBeforeTable) {
    const auto document = hps::parse("<div><table><b>alpha<tr><td>beta</td></tr></table></div>");
    ASSERT_NE(document, nullptr);

    const auto* div = document->querySelector("div");
    ASSERT_NE(div, nullptr);

    const auto children = div->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "b");
    EXPECT_EQ(children[0]->as_element()->text_content(), "alpha");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "table");

    const auto* cell = document->querySelector("table tbody tr td");
    ASSERT_NE(cell, nullptr);
    EXPECT_EQ(cell->text_content(), "beta");
}

TEST(HTMLParser, ParseFragmentTableContextFosterParentsElementInsideFragment) {
    const auto document = hps::parse_fragment("<b>alpha<tr><td>beta", "table");
    ASSERT_NE(document, nullptr);

    const auto children = document->children();
    ASSERT_EQ(children.size(), 2u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "b");
    EXPECT_EQ(children[0]->as_element()->text_content(), "alpha");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "tbody");
}

TEST(HTMLParser, RecoversMisnestedFormattingElements) {
    const auto document = hps::parse("<p><b><i>x</b>y</i>z");
    ASSERT_NE(document, nullptr);

    const auto* paragraph = document->querySelector("p");
    ASSERT_NE(paragraph, nullptr);

    const auto children = paragraph->children();
    ASSERT_EQ(children.size(), 3u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    ASSERT_TRUE(children[2]->is_text());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "b");
    EXPECT_EQ(children[0]->as_element()->text_content(), "x");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "i");
    EXPECT_EQ(children[1]->as_element()->text_content(), "y");
    EXPECT_EQ(children[2]->as_text()->value(), "z");
}

TEST(HTMLParser, ParseFragmentRecoversMisnestedFormattingElements) {
    const auto document = hps::parse_fragment("<b><i>x</b>y</i>z", "div");
    ASSERT_NE(document, nullptr);

    const auto children = document->children();
    ASSERT_EQ(children.size(), 3u);
    ASSERT_TRUE(children[0]->is_element());
    ASSERT_TRUE(children[1]->is_element());
    ASSERT_TRUE(children[2]->is_text());
    EXPECT_EQ(children[0]->as_element()->tag_name(), "b");
    EXPECT_EQ(children[0]->as_element()->text_content(), "x");
    EXPECT_EQ(children[1]->as_element()->tag_name(), "i");
    EXPECT_EQ(children[1]->as_element()->text_content(), "y");
    EXPECT_EQ(children[2]->as_text()->value(), "z");
}

TEST(HTMLParser, ParsedBooleanAttributesPreserveHasValue) {
    const auto document = hps::parse("<input checked data-empty=\"\">");
    ASSERT_NE(document, nullptr);

    const auto* input = document->querySelector("input");
    ASSERT_NE(input, nullptr);
    ASSERT_EQ(input->attribute_count(), 2u);

    EXPECT_EQ(input->attributes()[0].name(), "checked");
    EXPECT_FALSE(input->attributes()[0].has_value());
    EXPECT_TRUE(input->attributes()[0].value().empty());

    EXPECT_EQ(input->attributes()[1].name(), "data-empty");
    EXPECT_TRUE(input->attributes()[1].has_value());
    EXPECT_TRUE(input->attributes()[1].value().empty());
}

TEST(HTMLParser, DecodeEntitiesOptionDecodesNamedAndNumericEntities) {
    hps::Options opts;
    opts.decode_entities = true;

    const auto document = hps::parse("<p>&amp;&lt;&gt;&quot;&apos;&nbsp;&#65;&#x41;</p>", opts);
    ASSERT_NE(document, nullptr);

    const auto* paragraph = document->querySelector("p");
    ASSERT_NE(paragraph, nullptr);
    EXPECT_EQ(paragraph->text_content(), "&<>\"' AA");
}

TEST(HTMLParser, EnforcesMaxDepthBySkippingTooDeepSubtrees) {
    hps::Options opts;
    opts.max_depth = 2;

    const auto res = hps::parse_with_error("<a><b><c>deep</c></b></a>", opts);
    ASSERT_NE(res.document, nullptr);

    EXPECT_TRUE(has_error_code(res.errors, hps::ErrorCode::TooDeep));
    EXPECT_NE(res.document->querySelector("a"), nullptr);
    EXPECT_NE(res.document->querySelector("b"), nullptr);
    EXPECT_EQ(res.document->querySelector("c"), nullptr);
}

TEST(HTMLParser, EnforcesMaxAttributesPerElement) {
    hps::Options opts;
    opts.max_attributes = 1;

    const auto res = hps::parse_with_error("<div a='1' b='2'></div>", opts);
    ASSERT_NE(res.document, nullptr);
    ASSERT_TRUE(has_error_code(res.errors, hps::ErrorCode::TooManyAttributes));

    const auto* div = res.document->querySelector("div");
    ASSERT_NE(div, nullptr);
    ASSERT_EQ(div->attribute_count(), 1u);
    EXPECT_EQ(div->attributes()[0].name(), "a");
    EXPECT_EQ(div->attributes()[0].value(), "1");
}

TEST(HTMLParser, EnforcesAttributeLengthLimits) {
    hps::Options opts;
    opts.max_attribute_name_length = 3;
    opts.max_attribute_value_length = 2;

    const auto res = hps::parse_with_error("<div long='1' ok='1234'></div>", opts);
    ASSERT_NE(res.document, nullptr);
    ASSERT_TRUE(has_error_code(res.errors, hps::ErrorCode::AttributeTooLong));

    const auto* div = res.document->querySelector("div");
    ASSERT_NE(div, nullptr);
    ASSERT_EQ(div->attribute_count(), 1u);
    EXPECT_EQ(div->attributes()[0].name(), "ok");
    EXPECT_EQ(div->attributes()[0].value(), "12");
}

TEST(HTMLParser, EnforcesMaxTextLength) {
    hps::Options opts;
    opts.max_text_length = 3;

    const auto res = hps::parse_with_error("<div>abcdef</div>", opts);
    ASSERT_NE(res.document, nullptr);
    ASSERT_TRUE(has_error_code(res.errors, hps::ErrorCode::TextTooLong));

    const auto* div = res.document->querySelector("div");
    ASSERT_NE(div, nullptr);
    EXPECT_EQ(div->text_content(), "abc");
}
