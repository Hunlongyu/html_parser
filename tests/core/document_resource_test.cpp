#include "hps/hps.hpp"

#include "hps/core/document.hpp"
#include "hps/core/element.hpp"

#include <gtest/gtest.h>

namespace hps::tests {

TEST(DocumentResourceTest, MetaAndResourcesCanBeExtracted) {
    const auto doc = hps::parse(R"(
        <html>
          <head>
            <title>Title</title>
            <meta name="description" content="desc"/>
            <meta property="og:title" content="ogt"/>
          </head>
          <body>
            <a href="https://example.com/a">A</a>
            <a href="/b">B</a>
            <img src="/img.png"/>
            <div id="x" class="c1 c2">X</div>
            <div class="c2">Y</div>
          </body>
        </html>
    )");
    ASSERT_NE(doc, nullptr);

    EXPECT_EQ(doc->title(), "Title");
    EXPECT_EQ(doc->get_meta_content("description"), "desc");
    EXPECT_EQ(doc->get_meta_property("og:title"), "ogt");

    const auto links = doc->get_all_links();
    EXPECT_EQ(links.size(), 2u);
    EXPECT_EQ(links[0], "https://example.com/a");
    EXPECT_EQ(links[1], "/b");

    const auto images = doc->get_all_images();
    ASSERT_EQ(images.size(), 1u);
    EXPECT_EQ(images[0], "/img.png");

    const auto* x = doc->get_element_by_id("x");
    ASSERT_NE(x, nullptr);
    EXPECT_EQ(x->text_content(), "X");

    const auto divs = doc->get_elements_by_tag_name("div");
    EXPECT_EQ(divs.size(), 2u);

    const auto c2s = doc->get_elements_by_class_name("c2");
    EXPECT_EQ(c2s.size(), 2u);

    EXPECT_NE(doc->querySelector("#x"), nullptr);
    EXPECT_EQ(doc->querySelectorAll(".c2").size(), 2u);
}

}  // namespace hps::tests
