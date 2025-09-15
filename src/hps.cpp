#include "hps/hps.hpp"

#include "hps/core/document.hpp"
#include "hps/parsing/html_parser.hpp"

namespace hps {

std::shared_ptr<Document> parse(const std::string_view html) {
    HTMLParser parser;
    return parser.parse(html);
}

std::shared_ptr<Document> parse_file(const std::string_view path) {
    HTMLParser parser;
    return parser.parse_file(path);
}

std::string version() {
    return version_string;
}
}  // namespace hps