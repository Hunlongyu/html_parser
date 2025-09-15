#pragma once

#include "hps/hps_fwd.hpp"
#include "hps/version.hpp"

namespace hps {
std::shared_ptr<Document> parse(std::string_view html);

std::shared_ptr<Document> parse(std::string_view html, const ParserOptions& options);

std::shared_ptr<Document> parse_file(std::string_view path);

std::shared_ptr<Document> parse_file(std::string_view path, const ParserOptions& options);

std::string version();
}  // namespace hps