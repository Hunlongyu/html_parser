#include "hps/hps.hpp"

#include "hps/core/document.hpp"
#include "hps/parsing/html_parser.hpp"
#include "hps/parsing/options.hpp"

namespace hps {

std::shared_ptr<Document> parse(const std::string_view html, const Options& options) {
    HTMLParser parser;
    return parser.parse(html, options);
}

ParseResult parse_with_error(const std::string_view html, const Options& options) {
    HTMLParser                      parser;
    const std::shared_ptr<Document> document = parser.parse(html, options);
    return ParseResult{.document = document, .errors = parser.get_errors()};
}

std::shared_ptr<Document> parse_fragment(const std::string_view html, const std::string_view context_tag, const Options& options) {
    HTMLParser parser;
    return parser.parse_fragment(html, context_tag, options);
}

ParseResult parse_fragment_with_error(const std::string_view html, const std::string_view context_tag, const Options& options) {
    HTMLParser                      parser;
    const std::shared_ptr<Document> document = parser.parse_fragment(html, context_tag, options);
    return ParseResult{.document = document, .errors = parser.get_errors()};
}

std::shared_ptr<Document> parse_bytes(const std::string_view bytes, const Options& options) {
    HTMLParser parser;
    return parser.parse_bytes(bytes, options);
}

ParseResult parse_bytes_with_error(const std::string_view bytes, const Options& options) {
    HTMLParser                      parser;
    const std::shared_ptr<Document> document = parser.parse_bytes(bytes, options);
    return ParseResult{.document = document, .errors = parser.get_errors()};
}

std::shared_ptr<Document> parse_file(const std::string_view path, const Options& options) {
    HTMLParser parser;
    return parser.parse_file(path, options);
}

ParseResult parse_file_with_error(const std::string_view path, const Options& options) {
    HTMLParser                      parser;
    const std::shared_ptr<Document> document = parser.parse_file(path, options);
    return ParseResult{.document = document, .errors = parser.get_errors()};
}

std::string version() {
    return std::string(version_string);
}

}  // namespace hps
