#pragma once

#include "hps/parsing/tokenizer.hpp"
#include "hps/parsing/tree_builder.hpp"

namespace hps {

class HTMLParser : public NonCopyable {
  public:
    HTMLParser()  = default;
    ~HTMLParser() = default;

    std::unique_ptr<Document> parse(std::string_view html);

    std::unique_ptr<Document> parse(std::string_view html, const ParserOptions& options);

    std::unique_ptr<Document> parse_file(std::string_view filePath);

    std::unique_ptr<Document> parse_file(std::string_view filePath, const ParserOptions& options);

    const std::vector<ParseError>& get_errors() const noexcept;

  private:
    std::vector<ParseError> m_errors;
};
}  // namespace hps