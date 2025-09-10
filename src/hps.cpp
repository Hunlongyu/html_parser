#include "hps/hps.hpp"

namespace hps {
std::unique_ptr<Document> parse(std::string_view html) {
    return parse(html, ErrorHandlingMode::Lenient);
}

std::unique_ptr<Document> parse(std::string_view html, ErrorHandlingMode mode) {
    HTMLParser parser;
    return parser.parse(html, mode);
}

std::unique_ptr<Document> parse_file(std::string_view path) {
    return parse_file(path, ErrorHandlingMode::Lenient);
}

std::unique_ptr<Document> parse_file(std::string_view path, ErrorHandlingMode mode) {
    HTMLParser parser;
    return parser.parse_file(path, mode);
}

std::string version() {
    return version_string;
}
}