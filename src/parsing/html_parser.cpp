#include "hps/parsing/html_parser.hpp"

#include <fstream>
#include <sstream>

namespace hps {
std::unique_ptr<Document> HTMLParser::parse(const std::string_view html) {
    return parse(html, ErrorHandlingMode::Lenient);
}

std::unique_ptr<Document> HTMLParser::parse(std::string_view html, ErrorHandlingMode mode) {
    m_parse_errors.clear();
    m_builder_errors.clear();
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
                    throw;
                    // throw ParseException(ParseException::ErrorCode::InvalidHTML, "Invalid HTML");
                }
            }

            if (token->type() == TokenType::DONE) {
                break;
            }
        }

        if (!builder.finish()) {
            if (mode == ErrorHandlingMode::Strict) {
                throw;
                // throw ParseException(ParseException::ErrorCode::InvalidHTML, "Invalid HTML");
            }
        }

        const auto& tokenizer_errors = tokenizer.get_errors();
        m_parse_errors.insert(m_parse_errors.end(), tokenizer_errors.begin(), tokenizer_errors.end());

        const auto& builder_errors = builder.errors();
        m_builder_errors.insert(m_builder_errors.end(), builder_errors.begin(), builder_errors.end());

        return document;

    } catch (const ParseException& e) {
        m_parse_errors.emplace_back(e.code(), e.what(), 0);
        if (mode == ErrorHandlingMode::Strict) {
            throw;
        }
        // 在宽松模式下返回空文档
        return std::make_unique<Document>(std::string(html));

    } catch (const BuilderException& e) {
        m_builder_errors.emplace_back(e.code(), e.what(), 0, 0);
        if (mode == ErrorHandlingMode::Strict) {
            throw;
        }
        // 在宽松模式下返回空文档
        return std::make_unique<Document>(std::string(html));

    } catch (const std::exception& e) {
        // m_parse_errors.emplace_back(ParseException::ErrorCode::UNKNOWN_ERROR, e.what(), 0);
        if (mode == ErrorHandlingMode::Strict) {
            throw;
            // throw ParseException(ParseException::ErrorCode::UNKNOWN_ERROR, e.what());
        }
        // 在宽松模式下返回空文档
        return std::make_unique<Document>(std::string(html));
    }
}

std::unique_ptr<Document> HTMLParser::parse_file(std::string_view filePath) {
    return parse_file(filePath, ErrorHandlingMode::Lenient);
}

std::unique_ptr<Document> HTMLParser::parse_file(std::string_view filePath, ErrorHandlingMode mode) {
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
        return parse(html_content, mode);

    } catch (const std::exception& e) {
        // m_parse_errors.emplace_back(ParseException::ErrorCode::FILE_READ_ERROR, "File read error: " + std::string(e.what()), 0);
        if (mode == ErrorHandlingMode::Strict) {
            throw;
            // throw ParseException(ParseException::ErrorCode::FILE_READ_ERROR, "Cannot read file: " + std::string(filePath));
        }
        // 在宽松模式下返回空文档
        return std::make_unique<Document>("");
    }
}

const std::vector<ParseError>& HTMLParser::get_parse_errors() const noexcept {
    return m_parse_errors;
}

const std::vector<BuilderError>& HTMLParser::get_builder_errors() const noexcept {
    return m_builder_errors;
}
}  // namespace hps