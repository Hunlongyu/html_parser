#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/core/text_node.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace hps::tests {

TEST(DocumentTest, SourceHtmlIsStored) {
    const std::string html = "<html></html>";
    const Document    doc(html);
    EXPECT_EQ(doc.source_html(), html);
}

TEST(DocumentTest, RootPrefersHtmlElement) {
    Document doc("");

    auto head = std::make_unique<Element>("head");
    auto html = std::make_unique<Element>("html");
    const auto* html_ptr = html.get();

    doc.add_child(std::move(head));
    doc.add_child(std::move(html));

    EXPECT_EQ(doc.html(), html_ptr);
    EXPECT_EQ(doc.root(), html_ptr);
}

TEST(DocumentTest, TextContentConcatenatesAllChildrenRecursively) {
    Document doc("");

    auto html = std::make_unique<Element>("html");
    auto body = std::make_unique<Element>("body");
    body->add_child(std::make_unique<TextNode>("Hello"));
    body->add_child(std::make_unique<Element>("span"));
    body->add_child(std::make_unique<TextNode>("World"));
    html->add_child(std::move(body));
    doc.add_child(std::move(html));

    EXPECT_EQ(doc.text_content(), "HelloWorld");
}

TEST(DocumentTest, CharsetParsesMetaVariants) {
    Document doc("");

    auto html = std::make_unique<Element>("html");
    auto head = std::make_unique<Element>("head");

    auto meta_equiv = std::make_unique<Element>("meta");
    meta_equiv->add_attribute("http-equiv", "content-type");
    meta_equiv->add_attribute("content", "text/html; Charset=UTF-8");

    auto meta_charset = std::make_unique<Element>("meta");
    meta_charset->add_attribute("charset", "  utf-8  ");

    head->add_child(std::move(meta_charset));
    head->add_child(std::move(meta_equiv));
    html->add_child(std::move(head));
    doc.add_child(std::move(html));

    EXPECT_EQ(doc.charset(), "utf-8");
    EXPECT_EQ(doc.charset(), "utf-8");
}

TEST(DocumentTest, TitleFindsFirstTitleElement) {
    Document doc("");
    auto     html = std::make_unique<Element>("html");
    auto     head = std::make_unique<Element>("head");

    auto title = std::make_unique<Element>("title");
    title->add_child(std::make_unique<TextNode>("MyTitle"));
    head->add_child(std::move(title));

    html->add_child(std::move(head));
    doc.add_child(std::move(html));

    EXPECT_EQ(doc.title(), "MyTitle");
    EXPECT_EQ(doc.title(), "MyTitle");
}

}  // namespace hps::tests

