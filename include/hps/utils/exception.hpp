#pragma once
#include <exception>
#include <string>
#include <system_error>

namespace hps {

enum class ErrorCode {
    Success = 0,
    InvalidHTML,
    InvalidSelector,
    OutOfMemory,
    ParseTimeout,
};

// 基础异常类
class HPSException : public std::exception {
  public:
    explicit HPSException(std::string message) : m_message(std::move(message)) {}
    const char* what() const noexcept override {
        return m_message.c_str();
    }

  protected:
    std::string m_message;
};

// 解析异常
class ParseException : public HPSException {
  public:
    enum class ErrorCode { UnexpectedEOF, InvalidToken, InvalidNesting, TooManyElements, TooDeep };

    ParseException(ErrorCode code, std::string message, size_t position = 0)
        : HPSException(std::move(message)), m_code(code), m_position(position) {}

    ErrorCode code() const noexcept {
        return m_code;
    }
    size_t position() const noexcept {
        return m_position;
    }

  private:
    ErrorCode m_code;
    size_t    m_position;
};

// 选择器语法异常
class QueryException : public HPSException {
  public:
    QueryException(std::string message, size_t position = 0)
        : HPSException(std::move(message)), m_position(position) {}

    size_t position() const noexcept {
        return m_position;
    }

  private:
    size_t m_position;
};

// 内存不足异常
class OutOfMemoryException : public HPSException {
  public:
    explicit OutOfMemoryException(std::string message = "Out of memory")
        : HPSException(std::move(message)) {}
};

}  // namespace hps