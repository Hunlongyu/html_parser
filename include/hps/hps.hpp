#pragma once

#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/hps_fwd.hpp"
#include "hps/query/query.hpp"
#include "hps/version.hpp"

namespace hps {
std::shared_ptr<Document> parse(std::string_view html);

std::shared_ptr<Document> parse(std::string_view html, const Options& options);

std::shared_ptr<Document> parse_file(std::string_view path);

std::shared_ptr<Document> parse_file(std::string_view path, const Options& options);

std::string version();
}  // namespace hps