#include "hps/parsing/html_parser.hpp"

#include "hps/core/document.hpp"

#include <fstream>
#include <sstream>

namespace hps {
std::unique_ptr<Document> HTMLParser::parse(const std::string_view html) {
    return parse(html, ParserOptions::lenient());
}

std::unique_ptr<Document> HTMLParser::parse(std::string_view html, const ParserOptions& options) {
    m_errors.clear();
    const auto mode = options.error_handling;
    try {
        auto        document = std::make_unique<Document>(std::string(html));
        TreeBuilder builder(document.get());
        Tokenizer   tokenizer(html, mode);

        while (true) {
            auto token = tokenizer.next_token();
            if (!token.has_value()) {
                break;
            }
            if (!builder.process_token(token.value())) {
                if (mode == ErrorHandlingMode::Strict) {
                    throw HPSException(ErrorCode::InvalidHTML, "Invalid HTML");
                }
            }

            if (token->type() == TokenType::DONE) {
                break;
            }
        }

        if (!builder.finish()) {
            if (mode == ErrorHandlingMode::Strict) {
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
        if (mode == ErrorHandlingMode::Strict) {
            throw;
        }
        // 在宽松模式下返回空文档
        return std::make_unique<Document>(std::string(html));

    } catch (const std::exception& e) {
        m_errors.emplace_back(ErrorCode::UnknownError, e.what(), 0);
        if (mode == ErrorHandlingMode::Strict) {
            throw HPSException(ErrorCode::UnknownError, e.what());
        }
        // 在宽松模式下返回空文档
        return std::make_unique<Document>(std::string(html));
    }
}

std::unique_ptr<Document> HTMLParser::parse_file(std::string_view filePath) {
    return parse_file(filePath, ParserOptions::lenient());
}

std::unique_ptr<Document> HTMLParser::parse_file(std::string_view filePath, const ParserOptions& options) {
    const auto mode = options.error_handling;
    try {
        // 读取文件内容
        const std::ifstream file{std::string(filePath)};
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + std::string(filePath));
        }

        // 读取整个文件内容
        std::stringstream buffer;
        buffer << file.rdbuf();
        const std::string html_content = buffer.str();

        // 调用字符串解析方法
        return parse(html_content, options);

    } catch (const std::exception& e) {
        m_errors.emplace_back(ErrorCode::FileReadError, "File read error: " + std::string(e.what()), 0);
        if (mode == ErrorHandlingMode::Strict) {
            throw HPSException(ErrorCode::FileReadError, "Cannot read file: " + std::string(filePath));
        }
        // 在宽松模式下返回空文档
        return std::make_unique<Document>("");
    }
}

const std::vector<ParseError>& HTMLParser::get_errors() const noexcept {
    return m_errors;
}
}  // namespace hps