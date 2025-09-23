#pragma once
#include <exception>
#include <string>

namespace hps {

// 统一的位置信息结构
struct Location {
    size_t position = 0;
    size_t line     = 1;
    size_t column   = 1;

    Location() = default;

    explicit Location(const size_t position, const size_t line = 1, const size_t column = 1)
        : position(position),
          line(line),
          column(column) {}

    static Location from_position(const std::string_view& source, const size_t position);
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
    // XPath 错误
    InvalidXPath,
    XPathParseError,
    XPathEvaluationError,
};

// 错误信息结构体
struct HPSError {
    ErrorCode   code;
    std::string message;
    Location    location;

    HPSError(const ErrorCode code, std::string message, const size_t position)
        : code(code),
          message(std::move(message)),
          location(position) {}

    HPSError(const ErrorCode code, std::string message, const Location& loc = {})
        : code(code),
          message(std::move(message)),
          location(loc) {}
};

// 解析异常
class HPSException : public std::exception {
  public:
    explicit HPSException(const HPSError& error)
        : m_error(error) {}

    explicit HPSException(HPSError&& error)
        : m_error(std::move(error)) {}

    HPSException(const ErrorCode code, std::string message, const Location& location = {})
        : m_error(code, std::move(message), location) {}

    HPSException(const ErrorCode code, std::string message, const size_t position)
        : m_error(code, std::move(message), position) {}

    [[nodiscard]] ErrorCode code() const noexcept {
        return m_error.code;
    }

    [[nodiscard]] const char* what() const noexcept override {
        return m_error.message.c_str();
    }

    [[nodiscard]] const std::string& message() const noexcept {
        return m_error.message;
    }

    [[nodiscard]] const Location& location() const noexcept {
        return m_error.location;
    }

    [[nodiscard]] size_t position() const noexcept {
        return m_error.location.position;
    }

    [[nodiscard]] size_t line() const noexcept {
        return m_error.location.line;
    }

    [[nodiscard]] size_t column() const noexcept {
        return m_error.location.column;
    }

    [[nodiscard]] const HPSError& error() const noexcept {
        return m_error;
    }

  private:
    HPSError m_error;
};
}  // namespace hps