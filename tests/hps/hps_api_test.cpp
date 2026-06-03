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

    const auto* cell = result.document->query_selector("tbody tr td");
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

TEST(HPSApiTest, ParseFileTranscodesGbkInStrictMode) {
    const auto tmp = std::filesystem::temp_directory_path() / "hps__tmp__parse_file_gbk.html";
    {
        std::ofstream out(tmp, std::ios::binary);
        ASSERT_TRUE(out.is_open());
        out << "<html><head><meta charset='gbk'></head><body><p id='p'>";
        out.write("\xD6\xD0\xCE\xC4", 4);  // GBK 编码的“中文”
        out << "</p></body></html>";
    }

    const auto opts = Options::strict();
    const auto doc  = hps::parse_file(tmp.string(), opts);
    ASSERT_NE(doc, nullptr);
    const auto* p = doc->get_element_by_id("p");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->text_content(), "中文");

    std::error_code ec;
    std::filesystem::remove(tmp, ec);
}

TEST(HPSApiTest, ParseFileThrowsUnsupportedEncodingInStrictMode) {
    // euc-jp 可被嗅探出声明，但不在受支持的转码后端内 → UnsupportedEncoding。
    const auto tmp = std::filesystem::temp_directory_path() / "hps__tmp__parse_file_eucjp.html";
    {
        std::ofstream out(tmp, std::ios::binary);
        ASSERT_TRUE(out.is_open());
        out << "<html><head><meta charset='euc-jp'></head><body><p>x</p></body></html>";
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

TEST(HPSApiTest, ParseBytesPassesThroughUtf8) {
    const auto doc = hps::parse_bytes("<div id='a'>hello</div>");
    ASSERT_NE(doc, nullptr);
    const auto* div = doc->get_element_by_id("a");
    ASSERT_NE(div, nullptr);
    EXPECT_EQ(div->text_content(), "hello");
}

TEST(HPSApiTest, ParseBytesAutoDetectsDeclaredGbkCharset) {
    std::string bytes = "<html><head><meta charset='gb2312'></head><body><p id='p'>";
    bytes.append("\xD6\xD0\xCE\xC4", 4);  // GBK 编码的“中文”
    bytes += "</p></body></html>";

    const auto res = hps::parse_bytes_with_error(bytes);
    ASSERT_NE(res.document, nullptr);
    EXPECT_FALSE(res.has_errors());
    const auto* p = res.document->get_element_by_id("p");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->text_content(), "中文");
}

TEST(HPSApiTest, ParseBytesHonorsForcedEncodingOption) {
    // 无 meta 声明、纯字节体：依赖 Options::encoding 指定编码（如来自 HTTP Content-Type）。
    std::string bytes("\xD6\xD0\xCE\xC4", 4);  // GBK 编码的“中文”

    Options opts;
    opts.encoding = "GBK";  // 标签大小写/别名均可
    const auto doc = hps::parse_bytes(bytes, opts);
    ASSERT_NE(doc, nullptr);
    EXPECT_EQ(doc->text_content(), "中文");
}

TEST(HPSApiTest, ParseBytesReportsUndetectableEncodingInLenientMode) {
    // 无 BOM、无 meta、且不是合法 UTF-8 → 无法判定编码。
    std::string bytes("\xD6\xD0\xCE\xC4", 4);

    const auto res = hps::parse_bytes_with_error(bytes);
    ASSERT_NE(res.document, nullptr);
    EXPECT_TRUE(res.has_errors());
    EXPECT_EQ(res.errors[0].code, ErrorCode::UnsupportedEncoding);
}

TEST(HPSApiTest, VersionIsNotEmpty) {
    EXPECT_FALSE(hps::version().empty());
}

}  // namespace hps::tests
