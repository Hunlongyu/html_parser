#pragma once
#include <exception>
#include <string>
#include <system_error>

namespace hps {

// 统一的位置信息结构
struct SourceLocation {
    size_t position = 0;
    size_t line     = 1;
    size_t column   = 1;

    SourceLocation() = default;

    explicit SourceLocation(const size_t position, const size_t line = 1, const size_t column = 1) : position(position), line(line), column(column) {}

    static SourceLocation from_position(const std::string_view& source, const size_t position);
};

enum class ErrorCode {
    // 通用错误
    Success = 0,
    UnknownError,
    OutOfMemory,

    // Tokenizer 错误
    UnexpectedEOF,
    InvalidCharacter,
    InvalidToken,
    InvalidEntity,

    // TreeBuilder 错误
    InvalidNesting,
    UnclosedTag,
    VoidElementClose,
    MismatchedTag,
    TooManyElements,
    TooDeep,

    // Parser 错误
    InvalidHTML,
    ParseTimeout,
    QuirksMode,
    FileReadError,

    // Query 错误
    InvalidSelector,
    InvalidXPath,
};

// 错误信息结构体
struct ParseError {
    ErrorCode      code;
    std::string    message;
    SourceLocation location;

    ParseError(const ErrorCode code, std::string message, const size_t position) : code(code), message(std::move(message)), location(position) {}

    ParseError(const ErrorCode code, std::string message, const SourceLocation& loc = {}) : code(code), message(std::move(message)), location(loc) {}
};

// 解析异常
class HPSException : public std::exception {
  public:
    HPSException(const ErrorCode code, std::string message, const SourceLocation& location = {}) : m_code(code), m_message(std::move(message)), m_location(location) {}

    HPSException(const ErrorCode code, std::string message, const size_t position) : m_code(code), m_message(std::move(message)), m_location(position) {}

    [[nodiscard]] ErrorCode code() const noexcept {
        return m_code;
    }

    [[nodiscard]] const char* what() const noexcept override {
        return m_message.c_str();
    }

    [[nodiscard]] const std::string& message() const noexcept {
        return m_message;
    }

    [[nodiscard]] const SourceLocation& location() const noexcept {
        return m_location;
    }

    [[nodiscard]] size_t position() const noexcept {
        return m_location.position;
    }

    [[nodiscard]] size_t line() const noexcept {
        return m_location.line;
    }

    [[nodiscard]] size_t column() const noexcept {
        return m_location.column;
    }

  private:
    ErrorCode      m_code;
    std::string    m_message;
    SourceLocation m_location;
};
}  // namespace hps