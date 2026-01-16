#include "hps/parsing/options.hpp"
#include <gtest/gtest.h>

namespace hps::tests {

TEST(OptionsTest, DefaultValues) {
    Options opts;
    EXPECT_EQ(opts.error_handling, ErrorHandlingMode::Lenient);
    EXPECT_EQ(opts.comment_mode, CommentMode::Preserve);
    EXPECT_EQ(opts.whitespace_mode, WhitespaceMode::Preserve);
    EXPECT_EQ(opts.text_processing_mode, TextProcessingMode::Raw);
    EXPECT_EQ(opts.br_handling, BRHandling::Keep);
    EXPECT_EQ(opts.br_text, "\n");
    EXPECT_FALSE(opts.preserve_case);
    EXPECT_FALSE(opts.decode_entities);
    
    EXPECT_GT(opts.max_tokens, 0u);
    EXPECT_GT(opts.max_depth, 0u);
    EXPECT_TRUE(opts.is_valid());
}

TEST(OptionsTest, FactoryMethods) {
    auto strict = Options::strict();
    EXPECT_EQ(strict.error_handling, ErrorHandlingMode::Strict);
    EXPECT_TRUE(strict.is_valid());

    auto perf = Options::performance();
    EXPECT_EQ(perf.comment_mode, CommentMode::Remove);
    EXPECT_EQ(perf.whitespace_mode, WhitespaceMode::Remove);
    EXPECT_TRUE(perf.is_valid());

    auto sanitized = Options::sanitized();
    EXPECT_EQ(sanitized.comment_mode, CommentMode::Remove);
    EXPECT_EQ(sanitized.error_handling, ErrorHandlingMode::Lenient);
    EXPECT_TRUE(sanitized.is_valid());
}

TEST(OptionsTest, VoidElements) {
    Options opts;
    
    // Default void elements
    EXPECT_TRUE(opts.is_void_element("img"));
    EXPECT_TRUE(opts.is_void_element("br"));
    EXPECT_FALSE(opts.is_void_element("div"));
    
    // Custom void elements
    opts.add_void_element("custom-void");
    EXPECT_TRUE(opts.is_void_element("custom-void"));
    
    // Note: When custom elements are present, default elements are NOT checked
    // unless manually added. This is by design (if !empty return contains).
    
    opts.clear_void_elements();
    EXPECT_TRUE(opts.void_elements.empty());
    
    // Should fall back to defaults
    EXPECT_TRUE(opts.is_void_element("img"));
}

TEST(OptionsTest, ResetToDefaults) {
    Options opts = Options::strict();
    opts.reset_to_defaults();
    EXPECT_EQ(opts.error_handling, ErrorHandlingMode::Lenient);
}

} // namespace hps::tests
