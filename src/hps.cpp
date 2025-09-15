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

std::shared_ptr<Document> parse_file(const std::string_view path) {
    return parse_file(path, Options());
}

std::shared_ptr<Document> parse_file(const std::string_view path, const Options& options) {
    HTMLParser parser;
    return parser.parse_file(path, options);
}

std::string version() {
    return version_string;
}
}  // namespace hps