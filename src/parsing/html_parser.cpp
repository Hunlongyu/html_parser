#include "hps/parsing/html_parser.hpp"

#include "hps/core/document.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>

namespace hps {
std::shared_ptr<Document> HTMLParser::parse(const std::string_view html, const Options& options) {
    std::string owned_html(html);
    return parse_owned(std::move(owned_html), options);
}

std::shared_ptr<Document> HTMLParser::parse(std::string&& html, const Options& options) {
    return parse_owned(std::move(html), options);
}

std::shared_ptr<Document> HTMLParser::parse_owned(std::string html, const Options& options) {
    m_errors.clear();
    const auto error_handling = options.error_handling;

    if (!options.is_valid()) {
        m_errors.emplace_back(ErrorCode::InvalidHTML, "Invalid parser options", Location{});
        if (error_handling == ErrorHandlingMode::Strict) {
            throw HPSException(ErrorCode::InvalidHTML, "Invalid parser options");
        }
        return std::make_shared<Document>(std::move(html));
    }

    std::shared_ptr<Document> document;
    try {
        document = std::make_shared<Document>(std::move(html));
        TreeBuilder builder(document, options);
        Tokenizer   tokenizer(document->source_html(), options);

        size_t tokens_seen = 0;
        while (true) {
            auto token = tokenizer.next_token();
            if (!token.has_value()) {
                break;
            }

            ++tokens_seen;
            if (tokens_seen > options.max_tokens) {
                m_errors.emplace_back(ErrorCode::TooManyElements, "Token limit exceeded", tokenizer.position());
                if (error_handling == ErrorHandlingMode::Strict) {
                    throw HPSException(ErrorCode::TooManyElements, "Token limit exceeded", tokenizer.position());
                }
                break;
            }

            if (!token->is_done() && !builder.process_token(*token)) {
                if (error_handling == ErrorHandlingMode::Strict) {
                    throw HPSException(ErrorCode::InvalidHTML, "Invalid HTML", tokenizer.position());
                }
            }

            if (token->is_done()) {
                break;
            }
        }

        if (!builder.finish()) {
            if (error_handling == ErrorHandlingMode::Strict) {
                throw HPSException(ErrorCode::InvalidHTML, "Invalid HTML");
            }
        }

        auto tokenizer_errors = tokenizer.consume_errors();
        auto builder_errors   = builder.consume_errors();
        m_errors.reserve(m_errors.size() + tokenizer_errors.size() + builder_errors.size());
        m_errors.insert(m_errors.end(), std::make_move_iterator(tokenizer_errors.begin()), std::make_move_iterator(tokenizer_errors.end()));
        m_errors.insert(m_errors.end(), std::make_move_iterator(builder_errors.begin()), std::make_move_iterator(builder_errors.end()));

    } catch (const HPSException& e) {
        m_errors.push_back(e.error());
        if (error_handling == ErrorHandlingMode::Strict) {
            throw;
        }
    } catch (const std::exception& e) {
        m_errors.emplace_back(ErrorCode::UnknownError, e.what(), Location{});
        if (error_handling == ErrorHandlingMode::Strict) {
            throw HPSException(ErrorCode::UnknownError, e.what());
        }
    }

    if (!document) {
        document = std::make_shared<Document>(std::move(html));
    }
    return document;
}

std::shared_ptr<Document> HTMLParser::parse_file(const std::string_view filePath, const Options& options) {
    m_errors.clear();
    const auto mode = options.error_handling;
    try {
        const std::filesystem::path path(filePath);
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("File does not exist: " + path.string());
        }

        if (!std::filesystem::is_regular_file(path)) {
            throw std::runtime_error("Path is not a regular file: " + path.string());
        }

        std::ifstream file{path, std::ios::in | std::ios::binary};
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + path.string());
        }
        std::string     html_content;
        std::error_code ec;
        const auto      file_size = std::filesystem::file_size(path, ec);
        if (!ec && file_size > 0) {
            html_content.resize(static_cast<size_t>(file_size));
            file.read(html_content.data(), static_cast<std::streamsize>(file_size));
            if (!file && !file.eof()) {
                throw std::runtime_error("Cannot read file: " + path.string());
            }
        } else {
            std::stringstream buffer;
            buffer << file.rdbuf();
            html_content = buffer.str();
        }
        return parse_owned(std::move(html_content), options);

    } catch (const std::exception& e) {
        m_errors.emplace_back(ErrorCode::FileReadError, "File read error: " + std::string(e.what()), 0);
        if (mode == ErrorHandlingMode::Strict) {
            throw HPSException(ErrorCode::FileReadError, "Cannot read file: " + std::string(filePath));
        }
        // 在宽松模式下返回空文档
        return std::make_shared<Document>("");
    }
}

const std::vector<HPSError>& HTMLParser::get_errors() const noexcept {
    return m_errors;
}
}  // namespace hps
