#include "hps/parsing/html_parser.hpp"

#include "hps/core/document.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace hps {
std::shared_ptr<Document> HTMLParser::parse(const std::string_view html) {
    return parse(html, Options());
}

std::shared_ptr<Document> HTMLParser::parse(const std::string_view html, const Options& options) {
    m_errors.clear();
    const auto error_handling = options.error_handling;
    try {
        auto        document = std::make_shared<Document>(std::string(html));
        TreeBuilder builder(document, options);
        Tokenizer   tokenizer(html, options);

        while (true) {
            auto token = tokenizer.next_token();
            if (!token.has_value()) {
                break;
            }
            if (!builder.process_token(token.value())) {
                if (error_handling == ErrorHandlingMode::Strict) {
                    throw HPSException(ErrorCode::InvalidHTML, "Invalid HTML");
                }
            }

            if (token->type() == TokenType::DONE) {
                break;
            }
        }

        if (!builder.finish()) {
            if (error_handling == ErrorHandlingMode::Strict) {
                throw HPSException(ErrorCode::InvalidHTML, "Invalid HTML");
            }
        }

        const auto& tokenizer_errors = tokenizer.get_errors();
        m_errors.insert(m_errors.end(), tokenizer_errors.begin(), tokenizer_errors.end());

        const auto& builder_errors = builder.errors();
        m_errors.insert(m_errors.end(), builder_errors.begin(), builder_errors.end());

        return document;

    } catch (const HPSException& e) {
        m_errors.emplace_back(e.code(), e.what(), 0);
        if (error_handling == ErrorHandlingMode::Strict) {
            throw;
        }
        return std::make_shared<Document>(std::string(html));

    } catch (const std::exception& e) {
        m_errors.emplace_back(ErrorCode::UnknownError, e.what(), 0);
        if (error_handling == ErrorHandlingMode::Strict) {
            throw HPSException(ErrorCode::UnknownError, e.what());
        }
        return std::make_shared<Document>(std::string(html));
    }
}

std::shared_ptr<Document> HTMLParser::parse_file(const std::string_view filePath) {
    return parse_file(filePath, Options());
}

std::shared_ptr<Document> HTMLParser::parse_file(const std::string_view filePath, const Options& options) {
    const auto mode = options.error_handling;
    try {
        if (!std::filesystem::exists(filePath)) {
            throw std::runtime_error("File does not exist: " + std::string(filePath));
        }

        if (!std::filesystem::is_regular_file(filePath)) {
            throw std::runtime_error("Path is not a regular file: " + std::string(filePath));
        }

        const std::ifstream file{std::string(filePath)};
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + std::string(filePath));
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        const std::string html_content = buffer.str();
        return parse(html_content, options);

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