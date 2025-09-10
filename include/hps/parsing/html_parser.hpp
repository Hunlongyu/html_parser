#pragma once

#include "hps/parsing/tokenizer.hpp"
#include "hps/parsing/tree_builder.hpp"

namespace hps {

class HTMLParser : public NonCopyable {
  public:
    HTMLParser()  = default;
    ~HTMLParser() = default;

    std::unique_ptr<Document> parse(std::string_view html);

    std::unique_ptr<Document> parse(std::string_view html, ErrorHandlingMode mode);

    std::unique_ptr<Document> parse_file(std::string_view filePath);

    std::unique_ptr<Document> parse_file(std::string_view filePath, ErrorHandlingMode mode);

    const std::vector<ParseError>&   get_parse_errors() const noexcept;
    const std::vector<BuilderError>& get_builder_errors() const noexcept;

  private:
    std::vector<ParseError>   m_parse_errors;
    std::vector<BuilderError> m_builder_errors;
};
}  // namespace hps