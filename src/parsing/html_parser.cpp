#include "hps/parsing/html_parser.hpp"

#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/utils/encoding.hpp"
#include "hps/utils/string_utils.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace hps {
namespace {

[[nodiscard]] auto fragment_tokenizer_state_for_context(const std::string_view context_tag)
    -> TokenizerState {
    if (equals_ignore_case(context_tag, "svg")) {
        return TokenizerState::RAWTEXT;
    }
    if (equals_ignore_case(context_tag, "title") ||
        equals_ignore_case(context_tag, "textarea")) {
        return TokenizerState::RCDATA;
    }
    if (equals_ignore_case(context_tag, "style") ||
        equals_ignore_case(context_tag, "xmp") ||
        equals_ignore_case(context_tag, "iframe") ||
        equals_ignore_case(context_tag, "noembed") ||
        equals_ignore_case(context_tag, "noframes")) {
        return TokenizerState::RAWTEXT;
    }
    if (equals_ignore_case(context_tag, "script")) {
        return TokenizerState::ScriptData;
    }
    if (equals_ignore_case(context_tag, "plaintext")) {
        return TokenizerState::Plaintext;
    }
    return TokenizerState::Data;
}

[[nodiscard]] auto normalize_tag_name(
    std::string_view tag_name,
    const bool preserve_case) -> std::string {
    if (tag_name.empty()) {
        return "div";
    }
    std::string normalized(tag_name);
    if (!preserve_case) {
        std::ranges::transform(normalized, normalized.begin(), to_lower);
    }
    return normalized;
}

[[nodiscard]] auto namespace_for_context_tag(const std::string_view context_tag)
    -> NamespaceKind {
    if (equals_ignore_case(context_tag, "svg")) {
        return NamespaceKind::Svg;
    }
    if (equals_ignore_case(context_tag, "math")) {
        return NamespaceKind::MathML;
    }
    return NamespaceKind::Html;
}

[[nodiscard]] auto normalize_utf8_file_input(const std::string_view raw_bytes) -> std::string {
    if (raw_bytes.empty()) {
        return {};
    }

    const auto hint = sniff_html_encoding(raw_bytes);
    if (!hint.has_encoding()) {
        throw HPSException(
            ErrorCode::UnsupportedEncoding,
            "HTML file input must already be UTF-8");
    }

    if (hint.canonical_label != "utf-8") {
        const std::string detected =
            hint.detected_label.empty() ? hint.canonical_label : hint.detected_label;
        throw HPSException(
            ErrorCode::UnsupportedEncoding,
            "HTML file input must already be UTF-8; detected encoding: " + detected);
    }

    auto utf8 = decode_html_bytes_to_utf8(raw_bytes, hint.canonical_label);
    if (!utf8.has_value()) {
        throw HPSException(
            ErrorCode::UnsupportedEncoding,
            "HTML file input is not valid UTF-8");
    }

    return std::move(*utf8);
}

}  // namespace

std::shared_ptr<Document> HTMLParser::parse(const std::string_view html, const Options& options) {
    std::string owned_html(html);
    return parse_owned(std::move(owned_html), options);
}

std::shared_ptr<Document> HTMLParser::parse(std::string&& html, const Options& options) {
    return parse_owned(std::move(html), options);
}

std::shared_ptr<Document> HTMLParser::parse_fragment(
    const std::string_view html,
    const std::string_view context_tag,
    const Options& options) {
    std::string owned_html(html);
    return parse_fragment_owned(std::move(owned_html), context_tag, options);
}

std::shared_ptr<Document> HTMLParser::parse_fragment(
    std::string&& html,
    const std::string_view context_tag,
    const Options& options) {
    return parse_fragment_owned(std::move(html), context_tag, options);
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

    auto document = std::make_shared<Document>(std::move(html));
    try {
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
                m_errors.emplace_back(
                    ErrorCode::TooManyElements,
                    "Token limit exceeded",
                    tokenizer.position());
                if (error_handling == ErrorHandlingMode::Strict) {
                    throw HPSException(
                        ErrorCode::TooManyElements,
                        "Token limit exceeded",
                        tokenizer.position());
                }
                break;
            }

            if (!token->is_done() &&
                !builder.process_token(*token, tokenizer.position())) {
                if (error_handling == ErrorHandlingMode::Strict) {
                    throw HPSException(
                        ErrorCode::InvalidHTML,
                        "Invalid HTML",
                        tokenizer.position());
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
        m_errors.reserve(
            m_errors.size() + tokenizer_errors.size() + builder_errors.size());
        m_errors.insert(
            m_errors.end(),
            std::make_move_iterator(tokenizer_errors.begin()),
            std::make_move_iterator(tokenizer_errors.end()));
        m_errors.insert(
            m_errors.end(),
            std::make_move_iterator(builder_errors.begin()),
            std::make_move_iterator(builder_errors.end()));

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
    return document;
}

std::shared_ptr<Document> HTMLParser::parse_fragment_owned(
    std::string html,
    const std::string_view context_tag,
    const Options& options) {
    m_errors.clear();
    const auto error_handling = options.error_handling;

    if (!options.is_valid()) {
        m_errors.emplace_back(ErrorCode::InvalidHTML, "Invalid parser options", Location{});
        if (error_handling == ErrorHandlingMode::Strict) {
            throw HPSException(ErrorCode::InvalidHTML, "Invalid parser options");
        }
        return std::make_shared<Document>(std::move(html));
    }

    auto working_document = std::make_shared<Document>(std::move(html));
    const std::string normalized_context =
        normalize_tag_name(context_tag, options.preserve_case);

    try {
        auto  fragment_root = std::make_unique<Element>(
            normalized_context,
            namespace_for_context_tag(normalized_context));
        auto* fragment_element = const_cast<Element*>(
            working_document->add_child(std::move(fragment_root))->as_element());

        TreeBuilder builder(working_document, options, fragment_element);
        Tokenizer   tokenizer(
            working_document->source_html(),
            options,
            fragment_tokenizer_state_for_context(normalized_context),
            normalized_context);

        size_t tokens_seen = 0;
        while (true) {
            auto token = tokenizer.next_token();
            if (!token.has_value()) {
                break;
            }

            ++tokens_seen;
            if (tokens_seen > options.max_tokens) {
                m_errors.emplace_back(
                    ErrorCode::TooManyElements,
                    "Token limit exceeded",
                    tokenizer.position());
                if (error_handling == ErrorHandlingMode::Strict) {
                    throw HPSException(
                        ErrorCode::TooManyElements,
                        "Token limit exceeded",
                        tokenizer.position());
                }
                break;
            }

            if (!token->is_done() &&
                !builder.process_token(*token, tokenizer.position())) {
                if (error_handling == ErrorHandlingMode::Strict) {
                    throw HPSException(
                        ErrorCode::InvalidHTML,
                        "Invalid HTML fragment",
                        tokenizer.position());
                }
            }

            if (token->is_done()) {
                break;
            }
        }

        if (!builder.finish()) {
            if (error_handling == ErrorHandlingMode::Strict) {
                throw HPSException(ErrorCode::InvalidHTML, "Invalid HTML fragment");
            }
        }

        auto tokenizer_errors = tokenizer.consume_errors();
        auto builder_errors   = builder.consume_errors();
        m_errors.reserve(
            m_errors.size() + tokenizer_errors.size() + builder_errors.size());
        m_errors.insert(
            m_errors.end(),
            std::make_move_iterator(tokenizer_errors.begin()),
            std::make_move_iterator(tokenizer_errors.end()));
        m_errors.insert(
            m_errors.end(),
            std::make_move_iterator(builder_errors.begin()),
            std::make_move_iterator(builder_errors.end()));

        auto result_document =
            std::make_shared<Document>(std::string(working_document->source_html()));
        auto fragment_children = fragment_element->take_children();
        for (auto& child : fragment_children) {
            result_document->add_child(std::move(child));
        }
        return result_document;
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
    return std::make_shared<Document>(std::string(working_document->source_html()));
}

std::shared_ptr<Document> HTMLParser::parse_file(
    const std::string_view filePath,
    const Options& options) {
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

        std::string html_content;
        {
            std::error_code ec;
            const auto      file_size = std::filesystem::file_size(path, ec);
            if (!ec && file_size > 0) {
                html_content.reserve(static_cast<size_t>(file_size));
            }
        }
        html_content.assign(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>());
        if (!file.eof() && file.fail()) {
            throw std::runtime_error("Cannot read file: " + path.string());
        }
        html_content = normalize_utf8_file_input(html_content);
        return parse_owned(std::move(html_content), options);

    } catch (const HPSException& e) {
        m_errors.push_back(e.error());
        if (mode == ErrorHandlingMode::Strict) {
            throw;
        }
        return std::make_shared<Document>("");
    } catch (const std::exception& e) {
        m_errors.emplace_back(
            ErrorCode::FileReadError,
            "File read error: " + std::string(e.what()),
            0);
        if (mode == ErrorHandlingMode::Strict) {
            throw HPSException(
                ErrorCode::FileReadError,
                "Cannot read file: " + std::string(filePath));
        }
        return std::make_shared<Document>("");
    }
}

const std::vector<HPSError>& HTMLParser::get_errors() const noexcept {
    return m_errors;
}

}  // namespace hps
