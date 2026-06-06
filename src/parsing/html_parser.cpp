#include "hps/parsing/html_parser.hpp"

#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/parsing/tokenizer.hpp"
#include "hps/parsing/tree_builder.hpp"
#include "hps/utils/encoding.hpp"
#include "hps/utils/string_utils.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>

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

// 把转码结果映射为解析诊断；成功（含空输入）返回 std::nullopt。
// 编码细分原因（未知/不支持/非法字节）统一归到 UnsupportedEncoding 错误码，
// 但消息中给出精确语义；需要按状态分支处理的调用方应直接使用 hps::decode_to_utf8。
[[nodiscard]] auto decode_status_to_error(const DecodeResult& decoded)
    -> std::optional<HPSError> {
    const std::string label = decoded.encoding.empty() ? std::string("<unknown>") : decoded.encoding;
    switch (decoded.status) {
        case DecodeStatus::Ok:
            return std::nullopt;
        case DecodeStatus::UnknownEncoding:
            return HPSError{
                ErrorCode::UnsupportedEncoding,
                "Unable to detect input encoding; set Options::encoding "
                "or convert the input to UTF-8 first"};
        case DecodeStatus::UnsupportedEncoding:
            return HPSError{
                ErrorCode::UnsupportedEncoding,
                "Detected encoding is not supported for transcoding: " + label};
        case DecodeStatus::InvalidBytes:
            return HPSError{
                ErrorCode::UnsupportedEncoding,
                "Input bytes are not valid for encoding: " + label};
    }
    return std::nullopt;
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
        return std::make_shared<Document>(std::move(html), options.base_url);
    }

    auto document = std::make_shared<Document>(std::move(html), options.base_url);
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
        return std::make_shared<Document>(std::move(html), options.base_url);
    }

    auto working_document = std::make_shared<Document>(std::move(html), options.base_url);
    const std::string normalized_context =
        normalize_tag_name(context_tag, options.preserve_case);

    try {
        Element* fragment_element = working_document->create_element(
            normalized_context,
            namespace_for_context_tag(normalized_context));
        working_document->add_child(fragment_element);

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

        // arena 模型下，全部节点归 working_document 的 arena 所有；不能把它们搬进另一个
        // Document（会变悬空）。改为：把 fragment 内容从上下文元素上摘下，直接挂到 document 根下，
        // 并把上下文元素移出文档树（仍留在 arena 中，无害），返回 working_document 本身。
        auto fragment_children = fragment_element->take_children();
        working_document->take_children();  // 摘除上下文包裹元素
        for (auto* child : fragment_children) {
            working_document->add_child(child);
        }
        return working_document;
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
    return std::make_shared<Document>(std::string(working_document->source_html()), options.base_url);
}

std::shared_ptr<Document> HTMLParser::parse_bytes(
    const std::string_view bytes,
    const Options& options) {
    m_errors.clear();
    const auto mode = options.error_handling;

    auto decoded = decode_to_utf8(bytes, options.encoding);
    if (auto error = decode_status_to_error(decoded)) {
        m_errors.push_back(*error);
        if (mode == ErrorHandlingMode::Strict) {
            throw HPSException(*error);
        }
        return std::make_shared<Document>("");
    }

    // parse_owned 会重新 clear m_errors，此处转码已成功无需保留诊断。
    return parse_owned(std::move(decoded.text), options);
}

std::shared_ptr<Document> HTMLParser::parse_file(
    const std::string_view file_path,
    const Options& options) {
    m_errors.clear();
    const auto mode = options.error_handling;

    std::string raw_bytes;
    try {
        const std::filesystem::path path(file_path);
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

        {
            std::error_code ec;
            const auto      file_size = std::filesystem::file_size(path, ec);
            if (!ec && file_size > 0) {
                raw_bytes.reserve(static_cast<size_t>(file_size));
            }
        }
        raw_bytes.assign(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>());
        if (!file.eof() && file.fail()) {
            throw std::runtime_error("Cannot read file: " + path.string());
        }
    } catch (const std::exception& e) {
        m_errors.emplace_back(
            ErrorCode::FileReadError,
            "File read error: " + std::string(e.what()),
            0);
        if (mode == ErrorHandlingMode::Strict) {
            throw HPSException(
                ErrorCode::FileReadError,
                "Cannot read file: " + std::string(file_path));
        }
        return std::make_shared<Document>("");
    }

    // 检测/转码为 UTF-8（失败时记录 UnsupportedEncoding，严格模式下抛出）。
    auto decoded = decode_to_utf8(raw_bytes, options.encoding);
    if (auto error = decode_status_to_error(decoded)) {
        m_errors.push_back(*error);
        if (mode == ErrorHandlingMode::Strict) {
            throw HPSException(*error);
        }
        return std::make_shared<Document>("");
    }

    return parse_owned(std::move(decoded.text), options);
}

const std::vector<HPSError>& HTMLParser::get_errors() const noexcept {
    return m_errors;
}

}  // namespace hps
