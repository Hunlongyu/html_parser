#include "hps/hps.hpp"
#include "hps/core/document.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace hps::tests {

TEST(DocumentUrl, NoBaseReturnsRawLinks) {
    const auto doc = hps::parse(R"(<a href="page.html">x</a><a href="/abs">y</a>)");
    EXPECT_TRUE(doc->base_url().empty());
    EXPECT_EQ(doc->get_all_links(), (std::vector<std::string>{"page.html", "/abs"}));
}

TEST(DocumentUrl, OptionsBaseUrlResolvesLinks) {
    Options opts;
    opts.base_url  = "https://example.com/blog/post.html";
    const auto doc = hps::parse(
        R"(<a href="img.png">i</a><a href="/c">c</a><a href="http://x/y">a</a>)", opts);

    EXPECT_EQ(doc->base_url(), "https://example.com/blog/post.html");
    const auto links = doc->get_all_links();
    ASSERT_EQ(links.size(), 3u);
    EXPECT_EQ(links[0], "https://example.com/blog/img.png");
    EXPECT_EQ(links[1], "https://example.com/c");
    EXPECT_EQ(links[2], "http://x/y");
}

TEST(DocumentUrl, AbsoluteBaseHrefOverridesDocumentUrl) {
    Options opts;
    opts.base_url  = "https://example.com/blog/post.html";
    const auto doc = hps::parse(
        R"(<head><base href="https://cdn.example.com/assets/"></head><body><img src="logo.png"></body>)",
        opts);

    EXPECT_EQ(doc->base_url(), "https://cdn.example.com/assets/");
    const auto images = doc->get_all_images();
    ASSERT_EQ(images.size(), 1u);
    EXPECT_EQ(images[0], "https://cdn.example.com/assets/logo.png");
}

TEST(DocumentUrl, RelativeBaseHrefResolvedAgainstDocumentUrl) {
    Options opts;
    opts.base_url  = "https://example.com/a/b/page.html";
    const auto doc = hps::parse(R"(<base href="../assets/"><img src="x.png">)", opts);

    EXPECT_EQ(doc->base_url(), "https://example.com/a/assets/");
    ASSERT_FALSE(doc->get_all_images().empty());
    EXPECT_EQ(doc->get_all_images()[0], "https://example.com/a/assets/x.png");
}

TEST(DocumentUrl, ResolveUrlMethod) {
    Options opts;
    opts.base_url  = "https://example.com/dir/";
    const auto doc = hps::parse("<p>x</p>", opts);

    EXPECT_EQ(doc->resolve_url("a/b.html"), "https://example.com/dir/a/b.html");
    EXPECT_EQ(doc->resolve_url("../up"), "https://example.com/up");
    EXPECT_EQ(doc->resolve_url("#frag"), "https://example.com/dir/#frag");
    EXPECT_EQ(doc->resolve_url("https://other/x"), "https://other/x");
}

TEST(DocumentUrl, ParseBytesCarriesBaseUrl) {
    Options opts;
    opts.base_url  = "https://e.com/p/";
    const auto doc = hps::parse_bytes("<a href='x'>1</a>", opts);
    ASSERT_FALSE(doc->get_all_links().empty());
    EXPECT_EQ(doc->get_all_links()[0], "https://e.com/p/x");
}

}  // namespace hps::tests
