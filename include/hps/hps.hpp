#pragma once

#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/hps_fwd.hpp"
#include "hps/parsing/options.hpp"
#include "hps/query/query.hpp"
#include "hps/version.hpp"

#include "utils/exception.hpp"

namespace hps {

struct ParseResult {
    std::shared_ptr<Document> document;
    std::vector<HPSError>     errors;

    [[nodiscard]] bool has_errors() const noexcept {
        return !errors.empty();
    }

    [[nodiscard]] int error_count() const noexcept {
        return static_cast<int>(errors.size());
    }
};

std::shared_ptr<Document> parse(std::string_view html);

std::shared_ptr<Document> parse(std::string_view html, const Options& options);

ParseResult parse_with_error(std::string_view html);

ParseResult parse_with_error(std::string_view html, const Options& options);

std::shared_ptr<Document> parse_file(std::string_view path);

std::shared_ptr<Document> parse_file(std::string_view path, const Options& options);

ParseResult parse_file_with_error(std::string_view path);

ParseResult parse_file_with_error(std::string_view path, const Options& options);

std::string version();
}  // namespace hps