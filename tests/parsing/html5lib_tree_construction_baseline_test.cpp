#include "html5lib_tree_support.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#ifndef HPS_SOURCE_DIR
#error "HPS_SOURCE_DIR must be defined for baseline tests"
#endif

// 精选基线：仓库内自带的 tests/baselines/html5lib/tree-construction，逐例**严格**断言。
// （完整上游一致性套件由 html5lib_tree_construction_conformance_test.cpp 跑出通过率。）
TEST(HTML5LibTreeConstructionBaselineTest, CuratedDocumentCases) {
    namespace ts = hps::test_support;

    const std::filesystem::path baseline_dir =
        std::filesystem::path(HPS_SOURCE_DIR) / "tests" / "baselines" / "html5lib" / "tree-construction";
    const auto cases = ts::collect_cases_from_dir(baseline_dir);
    ASSERT_FALSE(cases.empty());

    hps::Options options;
    options.comment_mode    = hps::CommentMode::Preserve;
    options.whitespace_mode = hps::WhitespaceMode::Preserve;
    options.decode_entities = true;  // html5lib 期望树按规范解码字符引用

    size_t executed_cases = 0;
    for (const auto& test_case : cases) {
        if (!test_case.scripting_enabled) {
            continue;
        }

        SCOPED_TRACE(test_case.description);
        hps::ParseResult result;
        if (test_case.fragment_context.empty()) {
            result = hps::parse_with_error(test_case.input, options);
        } else {
            ASSERT_FALSE(test_case.expected_document.empty()) << test_case.fragment_context;
            result = hps::parse_fragment_with_error(
                test_case.input, ts::first_non_empty_line(test_case.fragment_context), options);
        }
        ASSERT_NE(result.document, nullptr);

        const std::string actual_tree = ts::serialize_document_tree(*result.document);
        EXPECT_EQ(actual_tree, test_case.expected_document);
        ++executed_cases;
    }

    EXPECT_GT(executed_cases, 0u);
}
