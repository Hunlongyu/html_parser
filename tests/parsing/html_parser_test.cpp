#include "hps/hps.hpp"

#include <filesystem>
#include <fstream>
#include <ranges>
#include <string>

#include <gtest/gtest.h>

namespace {
bool has_error_code(const std::vector<hps::HPSError>& errors, const hps::ErrorCode code) {
    return std::ranges::any_of(errors, [code](const hps::HPSError& err) { return err.code == code; });
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

    {
        std::ofstream out(temp_path, std::ios::out | std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out.write(html.data(), static_cast<std::streamsize>(html.size()));
    }

    const auto res = hps::parse_file_with_error(temp_path.string(), hps::Options{});
    ASSERT_NE(res.document, nullptr);
    EXPECT_EQ(res.document->source_html(), html);

    std::error_code ec;
    std::filesystem::remove(temp_path, ec);
}

