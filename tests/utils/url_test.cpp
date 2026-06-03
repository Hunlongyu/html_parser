#include "hps/utils/url.hpp"

#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace hps::tests {
namespace {
constexpr std::string_view kBase = "http://a/b/c/d;p?q";  // RFC 3986 §5.4 示例基准
}  // namespace

// ==================== RFC 3986 §5.4.1 普通示例 ====================

TEST(ResolveUrl, Rfc3986NormalExamples) {
    EXPECT_EQ(resolve_url(kBase, "g"), "http://a/b/c/g");
    EXPECT_EQ(resolve_url(kBase, "./g"), "http://a/b/c/g");
    EXPECT_EQ(resolve_url(kBase, "g/"), "http://a/b/c/g/");
    EXPECT_EQ(resolve_url(kBase, "/g"), "http://a/g");
    EXPECT_EQ(resolve_url(kBase, "//g"), "http://g");
    EXPECT_EQ(resolve_url(kBase, "?y"), "http://a/b/c/d;p?y");
    EXPECT_EQ(resolve_url(kBase, "g?y"), "http://a/b/c/g?y");
    EXPECT_EQ(resolve_url(kBase, "#s"), "http://a/b/c/d;p?q#s");
    EXPECT_EQ(resolve_url(kBase, "g#s"), "http://a/b/c/g#s");
    EXPECT_EQ(resolve_url(kBase, "g?y#s"), "http://a/b/c/g?y#s");
    EXPECT_EQ(resolve_url(kBase, ";x"), "http://a/b/c/;x");
    EXPECT_EQ(resolve_url(kBase, "g;x"), "http://a/b/c/g;x");
    EXPECT_EQ(resolve_url(kBase, ""), "http://a/b/c/d;p?q");
    EXPECT_EQ(resolve_url(kBase, "."), "http://a/b/c/");
    EXPECT_EQ(resolve_url(kBase, "./"), "http://a/b/c/");
    EXPECT_EQ(resolve_url(kBase, ".."), "http://a/b/");
    EXPECT_EQ(resolve_url(kBase, "../"), "http://a/b/");
    EXPECT_EQ(resolve_url(kBase, "../g"), "http://a/b/g");
    EXPECT_EQ(resolve_url(kBase, "../.."), "http://a/");
    EXPECT_EQ(resolve_url(kBase, "../../"), "http://a/");
    EXPECT_EQ(resolve_url(kBase, "../../g"), "http://a/g");
}

// ==================== RFC 3986 §5.4.2 异常示例 ====================

TEST(ResolveUrl, Rfc3986AbnormalExamples) {
    EXPECT_EQ(resolve_url(kBase, "../../../g"), "http://a/g");
    EXPECT_EQ(resolve_url(kBase, "../../../../g"), "http://a/g");
    EXPECT_EQ(resolve_url(kBase, "/./g"), "http://a/g");
    EXPECT_EQ(resolve_url(kBase, "/../g"), "http://a/g");
    EXPECT_EQ(resolve_url(kBase, "g."), "http://a/b/c/g.");
    EXPECT_EQ(resolve_url(kBase, ".g"), "http://a/b/c/.g");
    EXPECT_EQ(resolve_url(kBase, "g.."), "http://a/b/c/g..");
    EXPECT_EQ(resolve_url(kBase, "..g"), "http://a/b/c/..g");
    EXPECT_EQ(resolve_url(kBase, "g/./h"), "http://a/b/c/g/h");
    EXPECT_EQ(resolve_url(kBase, "g/../h"), "http://a/b/c/h");
}

TEST(ResolveUrl, AbsoluteReferenceIsReturnedAsIs) {
    EXPECT_EQ(resolve_url(kBase, "http://x/y"), "http://x/y");
    EXPECT_EQ(resolve_url(kBase, "https://example.com/a/b"), "https://example.com/a/b");
}

TEST(ResolveUrl, NonHttpSchemesPassThrough) {
    EXPECT_EQ(resolve_url(kBase, "mailto:user@example.com"), "mailto:user@example.com");
    EXPECT_EQ(resolve_url(kBase, "tel:+123456"), "tel:+123456");
    EXPECT_EQ(resolve_url(kBase, "javascript:void(0)"), "javascript:void(0)");
    EXPECT_EQ(resolve_url(kBase, "data:text/plain,hi"), "data:text/plain,hi");
}

TEST(ResolveUrl, ProtocolRelativeUsesBaseScheme) {
    EXPECT_EQ(resolve_url("https://site/page", "//cdn.example.com/lib.js"),
              "https://cdn.example.com/lib.js");
}

TEST(ResolveUrl, EmptyBaseReturnsReferenceUnchanged) {
    EXPECT_EQ(resolve_url("", "../a/b"), "../a/b");
    EXPECT_EQ(resolve_url("", "/abs"), "/abs");
    EXPECT_EQ(resolve_url("", "http://x/y"), "http://x/y");
}

TEST(ResolveUrl, RealWorldRelativePaths) {
    EXPECT_EQ(resolve_url("https://example.com/blog/post.html", "img/pic.png"),
              "https://example.com/blog/img/pic.png");
    EXPECT_EQ(resolve_url("https://example.com/blog/post.html", "../about.html"),
              "https://example.com/about.html");
    EXPECT_EQ(resolve_url("https://example.com/blog/post.html", "/contact"),
              "https://example.com/contact");
    EXPECT_EQ(resolve_url("https://example.com", "page"),
              "https://example.com/page");  // base 权威 + 空路径 → "/page"
}

TEST(IsAbsoluteUrl, DetectsScheme) {
    EXPECT_TRUE(is_absolute_url("http://x"));
    EXPECT_TRUE(is_absolute_url("https://x/y"));
    EXPECT_TRUE(is_absolute_url("mailto:a@b"));
    EXPECT_TRUE(is_absolute_url("data:,x"));

    EXPECT_FALSE(is_absolute_url("//cdn/x"));     // 协议相对，无 scheme
    EXPECT_FALSE(is_absolute_url("/a/b"));
    EXPECT_FALSE(is_absolute_url("a/b"));
    EXPECT_FALSE(is_absolute_url("../a"));
    EXPECT_FALSE(is_absolute_url(""));
    EXPECT_FALSE(is_absolute_url("1http://x"));   // scheme 不能以数字开头
}

}  // namespace hps::tests
