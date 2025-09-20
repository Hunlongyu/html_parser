#include "hps/hps.hpp"

#include "hps/core/document.hpp"
#include "hps/parsing/html_parser.hpp"
#include "hps/parsing/options.hpp"

namespace hps {
std::shared_ptr<Document> parse(const std::string_view html) {
    return parse(html, Options());
}

std::shared_ptr<Document> parse(const std::string_view html, const Options& options) {
    HTMLParser parser;
    return parser.parse(html, options);
}

ParseResult parse_with_error(const std::string_view html) {
    return parse_with_error(html, Options());
}

ParseResult parse_with_error(const std::string_view html, const Options& options) {
    HTMLParser                      parser;
    const std::shared_ptr<Document> document = parser.parse(html, options);
    const std::vector<HPSError>     errors   = parser.get_errors();

    return ParseResult{.document = document, .errors = errors};
}

std::shared_ptr<Document> parse_file(const std::string_view path) {
    return parse_file(path, Options());
}

std::shared_ptr<Document> parse_file(const std::string_view path, const Options& options) {
    HTMLParser parser;
    return parser.parse_file(path, options);
}

ParseResult parse_file_with_error(const std::string_view path) {
    return parse_file_with_error(path, Options());
}

ParseResult parse_file_with_error(const std::string_view path, const Options& options) {
    HTMLParser                      parser;
    const std::shared_ptr<Document> document = parser.parse_file(path, options);
    const std::vector<HPSError>     errors   = parser.get_errors();

    return ParseResult{.document = document, .errors = errors};
}

std::string version() {
    return version_string;
}
}  // namespace hps