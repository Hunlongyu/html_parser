#include "hps/hps.hpp"

#include "hps/core/element.hpp"
#include "hps/parsing/options.hpp"
#include "hps/utils/exception.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace hps::tests {

TEST(HPSApiTest, ParseDelegatesToParser) {
    const auto doc = hps::parse("<html><body><div id='a'>x</div></body></html>");
    ASSERT_NE(doc, nullptr);
    const auto* div = doc->get_element_by_id("a");
    ASSERT_NE(div, nullptr);
    EXPECT_EQ(div->tag_name(), "div");
    EXPECT_EQ(div->text_content(), "x");
}

TEST(HPSApiTest, ParseWithErrorReportsInvalidOptionsInLenientMode) {
    Options opts;
    opts.max_tokens = 0;
    const auto result = hps::parse_with_error("<html></html>", opts);
    ASSERT_NE(result.document, nullptr);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_EQ(result.errors[0].code, ErrorCode::InvalidHTML);
}

TEST(HPSApiTest, ParseFragmentBuildsFragmentWithoutHtmlShell) {
    const auto result = hps::parse_fragment_with_error("<tr><td>a<td>b", "table");
    ASSERT_NE(result.document, nullptr);
    EXPECT_TRUE(result.errors.empty());
    EXPECT_EQ(result.document->html(), nullptr);

    const auto* cell = result.document->querySelector("tbody tr td");
    ASSERT_NE(cell, nullptr);
    EXPECT_EQ(cell->text_content(), "a");
}

TEST(HPSApiTest, ParseFileWithErrorReportsMissingFileInLenientMode) {
    const auto missing = (std::filesystem::temp_directory_path() / "hps__missing__file__does_not_exist.html").string();
    const auto result = hps::parse_file_with_error(missing);
    ASSERT_NE(result.document, nullptr);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_EQ(result.errors[0].code, ErrorCode::FileReadError);
}

TEST(HPSApiTest, ParseFileReadsContent) {
    const auto tmp = std::filesystem::temp_directory_path() / "hps__tmp__parse_file.html";
    {
        std::ofstream out(tmp, std::ios::binary);
        ASSERT_TRUE(out.is_open());
        out << "<html><body><p id='p'>ok</p></body></html>";
    }

    Options opts;
    opts.error_handling = ErrorHandlingMode::Strict;
    const auto doc = hps::parse_file(tmp.string(), opts);
    ASSERT_NE(doc, nullptr);
    const auto* p = doc->get_element_by_id("p");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->text_content(), "ok");

    std::error_code ec;
    std::filesystem::remove(tmp, ec);
}

TEST(HPSApiTest, ParseFileThrowsInStrictModeWhenMissing) {
    const auto missing = (std::filesystem::temp_directory_path() / "hps__missing__strict.html").string();
    const auto opts = Options::strict();
    EXPECT_THROW((void)hps::parse_file(missing, opts), HPSException);
}

TEST(HPSApiTest, ParseFileThrowsUnsupportedEncodingInStrictMode) {
    const auto tmp = std::filesystem::temp_directory_path() / "hps__tmp__parse_file_gbk.html";
    {
        std::ofstream out(tmp, std::ios::binary);
        ASSERT_TRUE(out.is_open());
        out << "<html><head><meta charset='gbk'></head><body><p>";
        out.write("\xD6\xD0\xCE\xC4", 4);
        out << "</p></body></html>";
    }

    const auto opts = Options::strict();
    try {
        (void)hps::parse_file(tmp.string(), opts);
        FAIL() << "Expected unsupported encoding exception";
    } catch (const HPSException& ex) {
        EXPECT_EQ(ex.code(), ErrorCode::UnsupportedEncoding);
    }

    std::error_code ec;
    std::filesystem::remove(tmp, ec);
}

TEST(HPSApiTest, VersionIsNotEmpty) {
    EXPECT_FALSE(hps::version().empty());
}

}  // namespace hps::tests
